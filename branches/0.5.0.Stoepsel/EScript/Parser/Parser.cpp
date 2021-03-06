#include "Parser.h"
#include <iostream>
#include <stdio.h>
#include <stack>
#include "Tokenizer.h"
#include "Operators.h"
#include "../EScript.h"
#include "../Statements/FunctionCall.h"
#include "../Statements/GetAttribute.h"
#include "../Statements/SetAttribute.h"
#include "../Statements/IfControl.h"
#include "../Statements/WhileControl.h"
#include "../Statements/ForControl.h"
#include "../Statements/ForeachControl.h"
#include "../Statements/StateChangeControl.h"
#include "../Statements/LogicOp.h"
#include "../Statements/TryCatchControl.h"
#include "../Statements/Identifier.h"

#include "../Objects/Number.h"
#include "../Objects/Bool.h"
#include "../Objects/String.h"
#include "../Objects/UserFunction.h"

#include "../Utils/FileUtils.h"

using namespace EScript;
using std::string;
// -------------------------------------------------------------------------------------------------------------------
// helper

template<class BracketStart,class BracketEnd>
int findCorrespondingBracket(const Tokenizer::tokenList & tokens,int from,int to=-1,int direction=1){
	if (!Token::isA<BracketStart>(tokens[from])){
		std::cout << __FILE__<<":"<<__LINE__<<" should not happen!";
		return -1;
	}
	int cursor=from;
	int level=1;
	while(cursor != to){
		cursor+=direction;
		if (Token::isA<BracketStart>(tokens[cursor])){
			level++;
		}else if (Token::isA<BracketEnd>(tokens[cursor])){
			level--;
		}else if (Token::isA<TEndScript>(tokens[cursor])){
			return -1;
		}
		if (level==0)
			return cursor;
	}
	return -1;
}

// -------------------------------------------------------------------------------------------------------------------
Type* Parser::typeObject=NULL;
/**
 * [ESMF] Parser new Parser();
 */
ES_FUNCTION(esmf_constructor) {
    assertParamCount(runtime,parameter.count(),0,0);
    return new Parser();
}
/**
 * [ESMF] Block Parser.parse();
 */
ES_FUNCTION(esmf_parse) {
    assertParamCount(runtime,parameter.count(),1,1);
    ERef<Parser> p=new Parser();// assertType<Parser>(runtime,caller);

    ERef<Block> blockRef=new Block();

    try {
        p->parse(blockRef.get(),parameter[0]->toString().c_str());
    } catch (Object * e) {
//        Object::addReference(e);
        runtime.error("",e);
        return NULL;
//        throw e;
    }
    return blockRef.detachAndDecrease();
}
/**
 * [ESMF] Block Parser.parseFile();
 */
ES_FUNCTION(esmf_parseFile) {
    assertParamCount(runtime,parameter.count(),1,1);
    ERef<Parser> p=new Parser();// assertType<Parser>(runtime,caller);

    ERef<Block> blockRef=new Block();
    blockRef->setFilename(stringToIdentifierId(parameter[0]->toString()));

    try {
        p->parseFile(blockRef.get(),parameter[0]->toString().c_str());
    } catch (Object * e) {
        Object::addReference(e);
        throw e;
    }
    return blockRef.detachAndDecrease();
}

/**
 * initMembers
 */
void Parser::init(EScript::Namespace & globals) {
//
    // Parser ---|> [Object]
    typeObject=new Type(Object::getTypeObject());
    declareConstant(&globals,getClassName(),typeObject);

    declareFunction(typeObject,"_constructor",esmf_constructor);
    declareFunction(typeObject,"parse",esmf_parse);
    declareFunction(typeObject,"parseFile",esmf_parseFile);
}

// -------------------------------------------------------------------------------------------------------------------

/**
 * [ctor]
 */
Parser::Parser(Type * type):Object(type?type:typeObject) {
    //ctor
}

/**
 * [dtor]
 */
Parser::~Parser() {
    //dtor
}
/**
 * ---|> [Object]
 */
Object * Parser::clone()const {
    return new Parser();
}


/**
 *  Loads and parses a File.
 */
Object *  Parser::parseFile(Block * rootBlock,const char * filename)throw(Exception *) {
    char * buffer=NULL;
    long size=0;
    buffer=FileUtils::loadFile(filename,size);
    if (buffer==NULL)
        throw new Error(string("Could not open file:")+filename);

    tokenizer.defineToken("__FILE__",new TObject(String::create(filename)));
    tokenizer.defineToken("__DIR__",new TObject(String::create(FileUtils::dirname(filename))));

    currentFilename=stringToIdentifierId(filename);

    //    cout << buffer;
    Object *  s=NULL;
    try {
        s= parse(rootBlock,buffer);
    } catch (Exception * e) {
        e->setFilename(filename);
        delete [] buffer;
        throw(e);
    } catch (...) {
        std::cout << "!!!!";
    }

    delete [] buffer;
    return s;
}

/**
 *  Parse a CString.
 */
Object *  Parser::parse(Block * rootBlock,const char * c)throw(Exception *) {

    Tokenizer::tokenList * tokens=new Tokenizer::tokenList();
    /// 1. Tokenize
    try {
        tokenizer.getTokens(c,*tokens);
        pass_1(*tokens);
    } catch (Exception * e) {
        //std::cerr << e->toString() << std::endl;
        throw e;
    }

    /// 2. Parse definitions
    Tokenizer::tokenList * enrichedTokens=new Tokenizer::tokenList();
    pass_2(*tokens,rootBlock,*enrichedTokens);
    delete tokens;

    /// 3. Parse expressions and finally add them up the script.
    int cursor=0;
    Object * statement=this->getExpression(*enrichedTokens,cursor);

    /// 4. Delete the Tokens
    for (Tokenizer::tokenList::iterator it=enrichedTokens->begin();it!=enrichedTokens->end();++it) {
        Token::removeReference(*it);
    }

    return statement;
}

/**
 * [Helper]
 */
struct _BlockInfo {
    Token * token;
    unsigned int index;
    bool isCommandBlock;
    bool empty;
    bool containsColon;
    bool containsCommands;
    int shortIf; // a?b:c

    _BlockInfo(unsigned int _index=0,Token * _token=NULL):
            token(_token),index(_index),
            isCommandBlock(Token::isA<TStartBlock>(token)),empty(true),
            containsColon(false),containsCommands(false),shortIf(0) {};
    _BlockInfo(const _BlockInfo & b):
            token(b.token),index(b.index),
            isCommandBlock(b.isCommandBlock),empty(b.empty),
            containsColon(b.containsColon),containsCommands(b.containsCommands),shortIf(b.shortIf) {
//            std::cout << " ####### \n";
    };

};
/**
 * Pass 1
 * =========
 * - check Syntax of Brackets () [] {}
 * - disambiguate Map/Block
 * - colon ( Mapdelimiter / shortIf ?:)
 */
void Parser::pass_1(Tokenizer::tokenList & tokens)throw(Exception *) {

    std::stack<_BlockInfo> bInfStack;
    bInfStack.push(_BlockInfo());

    for(size_t cursor=0;cursor<tokens.size();++cursor) {
        Token * token=tokens.at(cursor);
        /// currentBlockInfo
        _BlockInfo & cbi=bInfStack.top();

		switch(token->getType()){
			case TStartBracket::TYPE_ID:
			case TStartBlock::TYPE_ID:
			case TStartIndex::TYPE_ID:{
				bInfStack.push(_BlockInfo(cursor,token));
				continue;
			}
			case TEndBracket::TYPE_ID:{
				if (!Token::isA<TStartBracket>(cbi.token)) {
					throw new Error("Syntax Error: ')'",token);
				}
				bInfStack.pop();
				continue;
			}
			case TEndIndex::TYPE_ID:{
				if (!Token::isA<TStartIndex>(cbi.token)) {
					throw new Error("Syntax Error: ']'",token);
				}
				bInfStack.pop();
				continue;
			}
			case TEndBlock::TYPE_ID:{
				if (!cbi.isCommandBlock) {
					throw new Error("Syntax Error: '}'",token);
				}
				/// Block is Map Constructor
				if ( cbi.containsColon) {
					unsigned int startIndex=cbi.index;

					Token * t=new TStartMap();
					if(dynamic_cast<Token*>(tokens.at(startIndex))!=NULL)
						t->setLine(dynamic_cast<Token*>(tokens.at(startIndex))->getLine());
					Token::removeReference(tokens.at(startIndex));

					tokens[startIndex]=t;
					Token::addReference(t);

					t=new TEndMap();
					if(dynamic_cast<Token*>(tokens.at(cursor))!=NULL)
						t->setLine(dynamic_cast<Token*>(tokens.at(cursor))->getLine());
					Token::removeReference(tokens.at(cursor));

					tokens[cursor]=t;
					Token::addReference(t);
				}
				bInfStack.pop();
				continue;
			}
			default:{
			}
		}

        if (cbi.isCommandBlock) {
            cbi.empty=false;
            if (Token::isA<TColon>(token) ){
                if (cbi.shortIf>0) {
                    cbi.shortIf--;
                } else if (cbi.containsCommands) {
                    throw new Error("Syntax Error in Block: ':'",token);
                } else {
                    cbi.containsColon=true;
                    Token * t=new TMapDelimiter();

                    if(dynamic_cast<Token*>(tokens.at(cursor))!=NULL)
						t->setLine(dynamic_cast<Token*>(tokens.at(cursor))->getLine());
                    Token::removeReference(tokens.at(cursor));

                    tokens[cursor]=t;
                    Token::addReference(t);
                    continue;
                }
            } else if (Token::isA<TEndCommand>(token)) {
                if (cbi.containsColon) {
                    throw new Error("Syntax Error in Map: ';'",token);
                }
                cbi.containsCommands=true;
                cbi.shortIf=0;
            } else if (Token::isA<TOperator>(token) && token->toString()=="?") {
				cbi.shortIf++;
            }
		}
    }
    //std::cout << "\n###"<<tStack.top()->toString();
    if (bInfStack.top().token!=NULL) {
        throw new Error("Unexpected eof (unclosed '"+bInfStack.top().token->toString()+"'?)",bInfStack.top().token);
    }
}

/**
 * Pass 2
 * =========
 * - Create Block-Objects
 * - Parse declarations
 * - Wrap parts of "fn" in brackets for easier processing: fn(foo,bar){...}  --->  fn( (foo,bar){} )
 * - TODO: Class declaration
 * - TODO: Check if already defined
 */
void Parser::pass_2(Tokenizer::tokenList & tokens,Block * root,
                    Tokenizer::tokenList & enrichedTokens)throw(Exception *) {

    std::stack<Block *> blockStack;
    blockStack.push(root);

	/// Counts the currently open brackets and blocks for the current function declaration.
	/// If the top value reaches 0 after reading a TEndBlock, the fn-wrapper brackets have to be closed.
	std::stack<int> functionBracketDepth;

	std::stack<TStartBracket*> currentBracket;

    enrichedTokens.reserve(tokens.size());

    TStartBlock * tsb=new TStartBlock(root);
    Token::addReference(tsb);
    enrichedTokens.push_back(tsb);

    for (size_t cursor=0;cursor<tokens.size();++cursor) {
        Token * token=tokens[cursor];
		switch(token->getType()){
			case TControl::TYPE_ID:{
				/// Variable Declaration
				TControl * tc=dynamic_cast<TControl *>(token);
				if (tc->getId()==Consts::IDENTIFIER_var) {
					if (TIdentifier * ti=(dynamic_cast<TIdentifier *>(tokens[cursor+1]))) {
						if(!blockStack.top()->declareVar(ti->getId())){
							std::cout << "\n Warning: Duplicate local variable '"<<ti->toString()<<"' ("<<getCurrentFilename()<<":"<<ti->getLine()<<")\n";
						}
						Token::removeReference(token);
						continue;
					} else
						throw new Error("var expects Identifier.",tc);
				}
				enrichedTokens.push_back(token);
				continue;
				/// name='static' ??????
			}
			/// Open new Block
			case TStartBlock::TYPE_ID:{
				TStartBlock * sb=dynamic_cast<TStartBlock *>(token);
				Block * currentBlock=new Block(sb->getLine());//currentBlock);

				/// debugging informations:
				currentBlock->setFilename(currentFilename);

				blockStack.push(currentBlock);
				sb->setBlock(currentBlock);
				enrichedTokens.push_back(token);

				if(!functionBracketDepth.empty())
					++functionBracketDepth.top();
				continue;
			}
			/// Close Block
			case TEndBlock::TYPE_ID:{
				enrichedTokens.push_back(token);

				blockStack.pop();
				if (blockStack.empty())
					throw new Error("Unexpected }");

				if(!functionBracketDepth.empty()){
					--functionBracketDepth.top();

					if(functionBracketDepth.top()==0){
						functionBracketDepth.pop();
						Token * t=new TEndBracket();
						t->setLine(dynamic_cast<Token *>(token)->getLine());

						Token::addReference(t);
						enrichedTokens.push_back(t);

						// add shortcut to the closing bracket
						currentBracket.top()->endBracketIndex=enrichedTokens.size()-1;
						currentBracket.pop();
					}
				}
				continue;
			}
			/// (
			case TStartBracket::TYPE_ID:{
				enrichedTokens.push_back(token);
				if(!functionBracketDepth.empty())
					++functionBracketDepth.top();
				currentBracket.push(dynamic_cast<TStartBracket*>(token));
				continue;
			}
			/// )
			case TEndBracket::TYPE_ID:{
				enrichedTokens.push_back(token);
				if(!functionBracketDepth.empty())
					--functionBracketDepth.top();

				if(currentBracket.empty())
					throw new Error("Missing opening bracket for ",token);

				// add shortcut to the closing bracket
				currentBracket.top()->endBracketIndex=enrichedTokens.size()-1;
				currentBracket.pop();
				continue;
			}

			/// fn(foo,bar){...}  ---> fn( (foo,bar){} )
			case TOperator::TYPE_ID:{
				enrichedTokens.push_back(token);
				if( token->toString() == "fn" || token->toString() == "lambda") {
					functionBracketDepth.push(0);
					TStartBracket * t=new TStartBracket();
					t->setLine(dynamic_cast<TOperator *>(token)->getLine());
					currentBracket.push(t);
					Token::addReference(t);
					enrichedTokens.push_back(t);
				}
				continue;
			}
			/// End of script
			case TEndScript::TYPE_ID:{
				blockStack.pop();
				if (!blockStack.empty())
					throw new Error("Unclosed {");

				Token * t=new TEndBlock();
				t->setLine(dynamic_cast<Token *>(token)->getLine());
				Token::addReference(t);
				enrichedTokens.push_back(t);
				enrichedTokens.push_back(token);
				return;
			}
			/// ...
			default:{
				enrichedTokens.push_back(token);
			}
		}
    }
}


/**
 *
 * Object * getExpression(...)
 *
 *
 * Cursor is moved to the last position of the Expression.
 *
 */
Object * Parser::getExpression(Tokenizer::tokenList & tokens,int & cursor,int to) throw(Exception *) {
    if (cursor>=static_cast<int>(tokens.size())){
    	return NULL;
    }/// Commands: if(...){}
    else if (Token::isA<TControl>(tokens[cursor])) {
        return getControl(tokens,cursor);
    } /// Block: {...}
    else if (Token::isA<TStartBlock>(tokens[cursor])) {
        return getBlock(tokens,cursor);
    }

    /// If "to" is not given, search the end of the expression
    if (to==-1) {
        to=findExpression(tokens,cursor);
    }

    /// Only happens when searching for non existing Expression:
    ///  the empty side of binary Expression (empty)!a or a++(empty)
    if (to<cursor) {
        return NULL;
    }

    ///  Single Element
    /// -------------------
    else if (to==cursor) {
        Token *t =tokens[cursor];

        /// Empty Command
        if (Token::isA<TEndCommand>(t)) {
            return NULL;
        }else if(TObject * tObj=dynamic_cast<TObject *>(t)){
//        	std::cout << "found obj: "<<tObj->obj.get()->toString()<<"\n";
			return tObj->obj.get();
        }
        ///  Identifier
        /// "a" => "_.get('a')"
        else if (TIdentifier * ident=dynamic_cast<TIdentifier *>(t)) {
            return new GetAttribute(0,ident->getId());  // ID
        }
//        out(t);
		std::cout << t->getLine()<<"\n";
        throw(new Error("Unknown (or unimplemented) Token",t));
    }

    ///  Command ends with ;
    ///  "2;"
    /// ---------------------
    else if (Token::isA<TEndCommand>(tokens[to])) {
        Object * e=getExpression(tokens,cursor,to-1);
        cursor=to;
        return e;
    }

    /// Surrounded with Brackets
    /// "(a+2)"
    /// --------------------------
    else if (Token::isA<TStartBracket>(tokens[cursor]) &&
             Token::isA<TEndBracket>(tokens[to]) &&
             findCorrespondingBracket<TStartBracket,TEndBracket>(tokens,cursor,to,1)==to) {

        ++cursor;
        Object * innerExpression=getExpression(tokens,cursor,to-1);
        cursor=to;
        return innerExpression;
    }

    /// Map Constructor
    /// "{foo:bar,2:3}"
    /// --------------------------
    if (Token::isA<TStartMap>(tokens[cursor]) &&
        Token::isA<TEndMap>(tokens[to]) &&
        findCorrespondingBracket<TStartMap,TEndMap>(tokens,cursor,to,1) == to) {
        return getMap(tokens,cursor);
    }

    /// BinaryExpression
    /// "3+foo"
    /// --------------------------
    if (Object * obj=getBinaryExpression(tokens,cursor,to)) {
        return obj;
    }

    ///    Syntax Error
    /// --------------------
    else {
        std::cout << "\n Error "<<cursor<<" - "<<to<<" :";
        for (;cursor<=to;++cursor) {
            std::cout << tokens[cursor]->toString();
            // TODO:LINE!!!
        }
        throw(new Error("Syntax error",tokens[cursor]));
    }
}

/**
 * Get block of statements
 * {out("foo");exit;}
 */
Object * Parser::getBlock(Tokenizer::tokenList & tokens,int & cursor) throw(Exception *) {
    TStartBlock * tsb=dynamic_cast<TStartBlock *>(tokens[cursor]);
    Block * b=tsb?reinterpret_cast<Block *>(tsb->getBlock()):NULL;
    if (b==NULL)
        throw new Error("No Block!",tokens[cursor]);

    ++cursor;
    Object * exp=NULL;

    /// Read commands.
    while (!Token::isA<TEndBlock>(tokens[cursor])) {
        if (Token::isA<TEndScript>(tokens[cursor]))
            throw(new Error("Unclosed Block {...",tsb));

        exp=getExpression(tokens,cursor);

        if (exp)
            b->addStatement(exp);

        /// Commands have to end on ";" or "}".
        if (!(Token::isA<TEndCommand>(tokens[cursor]) || Token::isA<TEndBlock>(tokens[cursor]))) {
            std::cout << tokens[cursor]->toString();
            throw new Error("Syntax Error in Block.",tokens[cursor]);
        }
        ++cursor;
    }
    return b;
}

/*!	getMap */
Object * Parser::getMap(Tokenizer::tokenList & tokens,int & cursor) throw(Exception *) {
    if (!Token::isA<TStartMap>(tokens[cursor]))
        throw new Error("No Map!",tokens[cursor]);

    // for debugging
    int currentLine=-1;
    {
        Token * t=dynamic_cast<Token *>(tokens[cursor]);
        if (t)
            currentLine=t->getLine();
    }

    ++cursor;

    Object * exp=0;
    std::vector<ObjRef> paramExp;
    while (!Token::isA<TEndMap>(tokens[cursor])) {

        /// i) read Key

        /// Key is not present
        if (Token::isA<TMapDelimiter>(tokens[cursor])) {
            exp=Void::get();
        } /// Key is present
        else {
            exp=getExpression(tokens,cursor);
            ++cursor;
        }
        paramExp.push_back(exp);

        /// ii) read ":"
        if (!Token::isA<TMapDelimiter>(tokens[cursor])) {
			std::cout << tokens[cursor]->toString();
            throw new Error("Map: Expected : ",tokens[cursor]);
        }
        ++cursor;

        /// iii) read Value
        /// Value is not present (only valid for last tuple)
        if (Token::isA<TEndMap>(tokens[cursor])) {
            exp=Void::get();
        } /// Value is present
        else {
            exp=getExpression(tokens,cursor);
            ++cursor;
        }
        paramExp.push_back(exp);

        if (Token::isA<TEndMap>(tokens[cursor]))
            break;
        else if (Token::isA<TDelimiter>(tokens[cursor])) {
            ++cursor;
            continue;
        } else
            throw new Error("Map Syntax Error",tokens[cursor]);
    }

    FunctionCall * funcCall = new FunctionCall(
					new GetAttribute(new GetAttribute(0,stringToIdentifierId("Map"),true),Consts::IDENTIFIER_fn_constructor),
					paramExp,false,currentFilename,currentLine);
    return funcCall;
}

/*!	Binary expression	*/
Object * Parser::getBinaryExpression(Tokenizer::tokenList & tokens,int & cursor,int to) throw(Exception *) {
    int currentLine=-1;
    {
        Token * t=dynamic_cast<Token *>(tokens[cursor]);
        if (t)
            currentLine=t->getLine();
//        std::cout << currentLine <<" ";
    }

    int opPosition=-1; /// Position of operator with lowest precedence
    int opPrecedence=-1; /// Highest precedence
    const Operator * op=NULL;

    /// search operator with lowest precedence

    int level=0; /// BracketLevel ( ) [] {}
    for (int i=cursor;i<=to;++i) {
        Token * t=tokens[i];
        if (level==0) {
            TOperator * top=dynamic_cast<TOperator *>(t);
            if (top &&
                    (top->getAssociativity()==Operator::L?
                     top->getPrecedence() >= opPrecedence :
                     top->getPrecedence() > opPrecedence)) {
                opPrecedence=top->getPrecedence();
                opPosition=i;
                op=top->getOperator();
                currentLine=top->getLine();
            }
        }
        if (Token::isA<TStartBlock>(t)
                ||Token::isA<TStartBracket>(t)
                ||Token::isA<TStartIndex>(t)
                ||Token::isA<TStartMap>(t)) {
            level++;
            continue;
        } else if (Token::isA<TEndBlock>(t)
                   ||Token::isA<TEndBracket>(t)
                   ||Token::isA<TEndIndex>(t)
                   ||Token::isA<TEndMap>(t)) {
            level--;
            if (level<0) {
                throw(new Error("Error in binary expression",t));
            }
            continue;

        }
    }
    if (opPosition<0 || !op) return NULL;

    int rightExprFrom=opPosition+1;
    int leftExprFrom=cursor,leftExprTo=opPosition-1;

    /// ASSIGNMENTS ( "="  ":=" )
    /// -----------
    if (op->getString()=="=") {
        identifierId memberIdentifier;
        Object * obj=NULL;
        Object * indexExp=NULL;
        int lValueType=getLValue(tokens,leftExprFrom,leftExprTo,obj,memberIdentifier,indexExp);

        Object * rightExpression=getExpression(tokens,rightExprFrom,to);
        cursor=rightExprFrom;


        /// a=2 => _.[a] = 2
        if (lValueType== LVALUE_MEMBER) {
            return new SetAttribute(obj,memberIdentifier,rightExpression,SetAttribute::ASSIGN);
        }
        /// a[1]=2 =>  _.a._set(1, 2)
        else if (lValueType == LVALUE_INDEX) {
            std::vector<ObjRef> paramExp;
            paramExp.push_back(indexExp);
            paramExp.push_back(rightExpression);
            return new FunctionCall(new GetAttribute(obj,Consts::IDENTIFIER_fn_set),paramExp,false,currentFilename,currentLine);
        } else {
            std::cout << "\n Error = "<<cursor<<" - "<<to<<" :" << lValueType;
            throw(new Error("Syntax error before '=' ",tokens[opPosition]));
        }
    } else if (op->getString()==":=") {
        identifierId memberIdentifier;
        Object * obj=NULL;
        Object * indexExp=NULL;
        int lValueType=getLValue(tokens,leftExprFrom,leftExprTo,obj,memberIdentifier,indexExp);

        Object * rightExpression=getExpression(tokens,rightExprFrom,to);
        cursor=rightExprFrom;


        /// a:=2 => _.[a] := 2
        if (lValueType== LVALUE_MEMBER) {
        	if(obj==NULL){
				std::cout << "\n Warning: ':=' used for assigning to non member variable; use '=' instead! ("<<
						getCurrentFilename()<<":"<<currentLine<<")\n";//<<tokens[cursor]->getLine()<<")";
        	}
            return new SetAttribute(obj,memberIdentifier,rightExpression,SetAttribute::SET_OBJ_ATTRIBUTE);
        }
//        // a[1]=2 =>  _.a._set(1, 2)
//        else if (lValueType == LVALUE_INDEX) {
//            std::vector<Object *> * paramExp=new  std::vector<Object *>();
//            paramExp->push_back(indexExp);
//            paramExp->push_back(rightExpression);
//            return new FunctionCall(new GetAttribute(obj,Consts::IDENTIFIER_fn__set ),paramExp,false,currentLine);
//        }
        else {
            std::cout << "\n Error = "<<cursor<<" - "<<to<<" :" << lValueType;
            throw(new Error("Syntax error before ':=' ",tokens[opPosition]));
        }
    } else if (op->getString()=="::=") {
        identifierId memberIdentifier;
        Object * obj=NULL;
        Object * indexExp=NULL;
        int lValueType=getLValue(tokens,leftExprFrom,leftExprTo,obj,memberIdentifier,indexExp);

        Object * rightExpression=getExpression(tokens,rightExprFrom,to);
        cursor=rightExprFrom;

        /// a::=2 => _.[a] ::= 2
        if (lValueType== LVALUE_MEMBER) {
            return new SetAttribute(obj,memberIdentifier,rightExpression,SetAttribute::SET_TYPE_ATTRIBUTE);
        }
        else {
            std::cout << "\n Error = "<<cursor<<" - "<<to<<" :" << lValueType;
            throw(new Error("Syntax error before '::=' ",tokens[opPosition]));
        }
    }

    /// get left expression
    Object * leftExpression=getExpression(tokens,leftExprFrom,leftExprTo);

    /// "a.b.c"
    if (op->getString()==".") {
        if (rightExprFrom>to) {
            std::cout << "\n Error .1 "<<cursor<<" - "<<to<<" :";
            throw(new Error("Syntax error after '.'",tokens[opPosition]));
        }
        cursor=to;

        /// "a.b"
        if (Token::isA<TIdentifier>(tokens[rightExprFrom])){
            return new GetAttribute(leftExpression,dynamic_cast<TIdentifier *>(tokens[rightExprFrom])->getId());
        }
        /// "a.+"
        else if (Token::isA<TOperator>(tokens[rightExprFrom])) {
            return new GetAttribute(leftExpression,dynamic_cast<TOperator *>(tokens[rightExprFrom])->toString());
        }
        else if(Token::isA<TObject>(tokens[rightExprFrom])){
			Object * obj=dynamic_cast<TObject*>(tokens[rightExprFrom])->obj.get();
			/// "a.'+'"
			if (String * s=dynamic_cast<String *>(obj)) {
				return new GetAttribute(leftExpression,s->toString());
			}/// "a.$b"
			else if (Identifier * i=dynamic_cast<Identifier *>(obj)) {
				return new GetAttribute(leftExpression,i->getId());
			}
        }
		std::cout << "\n Error .2 "<<cursor<<" - "<<to<<" :";
		throw(new Error("Syntax error after '.'",tokens[opPosition]));
    }
    ///  Function Call
    /// "a(b)"  "a(1,2,3)"
    else if (op->getString()=="(") {
        cursor=rightExprFrom-1;
        std::vector<ObjRef> paramExp;
		getExpressionsInBrackets(tokens,cursor,paramExp);

        if(cursor!=to){
            throw new Error("Error after function call. Forgotten ';' ?",tokens[cursor]);
        }
        FunctionCall * funcCall = new FunctionCall(leftExpression,paramExp,false,currentFilename,currentLine);
        return funcCall;
    }
    ///  Index Exression | Array
    else if (op->getString()=="[") {
        /// No left expression present? -> Array-constructor
        ///"[1,a+2,3]" -> new Array(1,a+2,3)
        if (!leftExpression) {
            std::vector<ObjRef> paramExp;
            ++cursor;
            while (!Token::isA<TEndIndex>(tokens[cursor]) ) {

                paramExp.push_back(getExpression(tokens,cursor));

                ++cursor;
                if (Token::isA<TDelimiter>(tokens[cursor]))
                    ++cursor;
                else if (!Token::isA<TEndIndex>(tokens[cursor])){
					std::cout << tokens[cursor]->toString();
                    throw new Error("Expected ]",tokens[opPosition]);
                }
            }
            cursor=to;
            FunctionCall * funcCall = new FunctionCall(new GetAttribute(new GetAttribute(0,"Array",true),Consts::IDENTIFIER_fn_constructor),
													paramExp,false,currentFilename,currentLine);
            return funcCall;
        }
        /// Left expression present? -> Index Expression
        /// "a[1]"
        cursor=rightExprFrom;
        std::vector<ObjRef> paramExp;
        paramExp.push_back(getExpression(tokens,cursor));
        cursor=to;
        FunctionCall * funcCall = new FunctionCall(new GetAttribute(leftExpression,Consts::IDENTIFIER_fn_get),paramExp,
										false,currentFilename,currentLine);
        return funcCall;

    }/// "a?1:2"
    else if (op->getString()=="?") {
        cursor=rightExprFrom;
        Object * alt1=getExpression(tokens,cursor);
        ++cursor;
        if (!Token::isA<TColon>(tokens[cursor])) {
            std::cout <<  tokens[cursor]->toString();
            throw new Error("Expected :",tokens[cursor]);
        }
        ++cursor;
        Object * alt2=getExpression(tokens,cursor);
        return new IfControl(leftExpression,alt1,alt2);
    } /// new Object
    else if (op->getString()=="new") {
        ++cursor;
        if (leftExpression)
            throw new Error("new is a unary left operator.",tokens[cursor]);

        int objExprTo=to;

        /// if new has paramteres "(...)", search for their beginning.
        if (Token::isA<TEndBracket>(tokens[objExprTo])) {
            objExprTo=findCorrespondingBracket<TEndBracket,TStartBracket>(tokens,objExprTo,rightExprFrom,-1);
        }
        /// read parameters
		std::vector<ObjRef> paramExp;
        if (objExprTo>cursor) {
            int cursor2=objExprTo;
			getExpressionsInBrackets(tokens,cursor2,paramExp);

            objExprTo--; /// why ?????????????
        }
        /// read Object-expression
        Object * obj=getExpression(tokens,cursor,objExprTo);
        cursor=to;

        return new FunctionCall(new GetAttribute(obj,Consts::IDENTIFIER_fn_constructor),paramExp,true,
									currentFilename,currentLine);
        // TODO: !!! Return this-reference !!! ???? What does this mean?
    }
    /// Function "fn(a,b){return a+b;}"
    else if (op->getString()=="fn" || op->getString()=="lambda") {
		Object * result=getFunctionDeclaration(tokens,cursor);
		if (cursor!=to)    {
            throw(new Error("[fn] Syntax error.",tokens[cursor]));
        }
        return result;
    }

    Object * rightExpression=getExpression(tokens,rightExprFrom,to);

    cursor=rightExprFrom;

    /// Unary prefix expression
    /// ++a, --a, !a
    /// Bsp.: ++a =>  _.a.++pre()
    if (! leftExpression) {
        /// +a  +3
        if (op->getString()=="+"){
            // @optimization
			if (Number* num=dynamic_cast<Number*>(rightExpression)) {
				return num;
			}
        }
        /// -a  -3
        else if (op->getString()=="-") {
            // @optimization
			if (Number* num=dynamic_cast<Number*>(rightExpression)) {
				Number * newNum=Number::create(-num->toDouble());
				delete num;
				return newNum ;
			}
//            if (Number* num=dynamic_cast<Number*>(rightExpression)) {
//                Number * newNum=Number::create(-num->toDouble());
//                delete num;
//                return newNum ;
//            }
        } else if (op->getString()=="!") {
            return new LogicOp(rightExpression,0,LogicOp::NOT);
        }

        //if (GetAttribute * ga=dynamic_cast<GetAttribute *>(rightExpression)) {
        FunctionCall * fc=new FunctionCall(
            new GetAttribute(rightExpression,
                             string(op->getString())+"_pre"),std::vector<ObjRef>(),false,currentFilename,currentLine);
        return  fc;

    } else
        /// Unary postfix expression
        /// a++, a--, a!
        /// Bsp: a++ => _.a.++post()
        if (!rightExpression) {
            //  if (GetAttribute * ga=dynamic_cast<GetAttribute *>(leftExpression)) {
            FunctionCall * fc=new FunctionCall(
                new GetAttribute(leftExpression,
                                 string(op->getString())+"_post"),std::vector<ObjRef>(),false,currentFilename,currentLine);
            cursor--;

            return  fc;
        }
    /// ||
        else if (op->getString()=="||") {
            return new LogicOp(leftExpression,rightExpression,LogicOp::OR);
        }
    /// &&
        else if (op->getString()=="&&") {
            return new LogicOp(leftExpression,rightExpression,LogicOp::AND);
        }
    /// normal binary expression
    /// 1+2 -> 1.+(2)
        else {
            std::vector<ObjRef> paramExp;
            paramExp.push_back(rightExpression);
            FunctionCall * funcCall = new FunctionCall(new GetAttribute(leftExpression, op->getString()),paramExp,
													false,currentFilename,currentLine);
            return funcCall;
        }
    return NULL;
}

/*!	Read a function declaration. Must begin with "fn" or "lambda".
	Cursor is placed at the end of the block.
	\note after pass_2(...) a function looks like this:
			fn( (params*) {...} )  OR
			fn( (params*).(constrExpr) {...} )
	*/
Object * Parser::getFunctionDeclaration(Tokenizer::tokenList & tokens,int & cursor) throw (Exception *){
	bool lambda=false;
	Token * t=tokens[cursor];

	if(t->toString()=="lambda"){
		lambda=true;
	}else if(t->toString()!="fn"){
		throw new Error("No function! ",tokens[cursor]);
	}
	++cursor;

	/// step over '(' inserted at pass_2(...)
	++cursor;

	UserFunction::parameterList_t * params=getFunctionParameters(tokens,cursor);
	TOperator * superOp=dynamic_cast<TOperator*>(tokens[cursor]);

	/// fn(a).(a+1,2){}
	std::vector<ObjRef> superConCallExpressions;
	if(superOp!=NULL && superOp->toString()=="."){
		++cursor;
		getExpressionsInBrackets(tokens,cursor,superConCallExpressions);
		++cursor; // step over ')'
//		std::cout << " #### ";
	}

	Block * block=dynamic_cast<Block*>(getExpression(tokens,cursor));
	if (block==NULL) {
		std::cout << tokens[cursor]->toString();
//
//		out(tokens[cursor]);
		throw new Error("[fn] Expects Block of statements.",tokens[cursor]);
	}
	/// step over ')' inserted at pass_2(...)
	++cursor;

	return new UserFunction(params,block,lambda,superConCallExpressions);
}


/**
 * Reads a Control-Statement from tokens beginning at index "cursor".
 * Cursor is placed at the last Token of the statement.
 * @param tokens Program as Token-List.
 * @param curosr Cursor pointing at current Token.
 * @return Control-statement or NULL if no Control-Statement could be read.
 */
Object * Parser::getControl(Tokenizer::tokenList & tokens,int & cursor)throw(Exception *) {

    TControl * tc=dynamic_cast<TControl *>(tokens[cursor]);
    if (!tc) return NULL;
    ++cursor;

	identifierId cId=tc->getId();
	/// if-Control
	if(cId==Consts::IDENTIFIER_if){
		if (!Token::isA<TStartBracket>(tokens[cursor]))
			throw(new Error("[if] expects (",tokens[cursor]));
		++cursor;
		Object * condition=getExpression(tokens,cursor);
		++cursor;
		if (!Token::isA<TEndBracket>(tokens[cursor])) {
			throw(new Error("[if] expects (...)",tokens[cursor]));
		}
		++cursor;
		Object * action=getExpression(tokens,cursor);
		Object * elseAction=0;
		if ((tc=dynamic_cast<TControl *>(tokens[cursor+1]))) {
			if (tc->getId()==Consts::IDENTIFIER_else) {
				++cursor;
				++cursor;
				elseAction=getExpression(tokens,cursor);
			}
		}
		return new IfControl(condition,action,elseAction);
	}
	/// for-Control
	else if(cId==Consts::IDENTIFIER_for) {
		if (!Token::isA<TStartBracket>(tokens[cursor]))
			throw(new Error("[for] expects (",tokens[cursor]));
		++cursor;
		Object * initExp=getExpression(tokens,cursor);
		if (!Token::isA<TEndCommand>(tokens[cursor])) {
            std::cout << tokens[cursor]->toString();
//			out(tokens[cursor]);
			throw(new Error("[for] expects ;",tokens[cursor]));
		}
		++cursor;
		Object * condition=getExpression(tokens,cursor);
		if (!Token::isA<TEndCommand>(tokens[cursor])) {
			throw(new Error("[for] expects ;",tokens[cursor]));
		}
		++cursor;
		Object * incr=getExpression(tokens,cursor);
		if (incr)
			++cursor;
		if (!Token::isA<TEndBracket>(tokens[cursor])) {
            std::cout << tokens[cursor]->toString();
//			out(tokens[cursor]);
			throw new Error("[for] expects )",tokens[cursor]);
		}
		++cursor;
		Object * action=getExpression(tokens,cursor);
		return new ForControl(initExp,condition,incr,action);
	}
	/// while-Control
	else if(cId==Consts::IDENTIFIER_while) {
		if (!Token::isA<TStartBracket>(tokens[cursor]))
			throw new Error("[while] expects (",tokens[cursor]);
		++cursor;
		Object * condition=getExpression(tokens,cursor);
		++cursor;
		if (!Token::isA<TEndBracket>(tokens[cursor])) {
			throw new Error("[while] expects (...)",tokens[cursor]);
		}
		++cursor;
		Object * action=getExpression(tokens,cursor);
		return new WhileControl(condition,action);
	}
	/// Do-while-Control
	else if(cId==Consts::IDENTIFIER_do) {
		Object * action=getExpression(tokens,cursor);
		++cursor;
		tc=dynamic_cast<TControl *>(tokens[cursor]);
		if (!tc || tc->getId()!=Consts::IDENTIFIER_while)
			throw new Error("[do-while] expects while",tokens[cursor]);
		++cursor;
		if (!Token::isA<TStartBracket>(tokens[cursor]))
			throw new Error("[do-while] expects (",tokens[cursor]);
		++cursor;
		Object * condition=getExpression(tokens,cursor);
		++cursor;
		if (!Token::isA<TEndBracket>(tokens[cursor])) {
			throw new Error("[do-while] expects (...)",tokens[cursor]);
		}
		++cursor;
		if (!Token::isA<TEndCommand>(tokens[cursor])) {
			throw new Error("[do-while] expects ;",tokens[cursor]);
		}
		return new WhileControl(condition,action,true);
	}
	/// foreach-Control
	else if(cId==Consts::IDENTIFIER_foreach) {
		if (!Token::isA<TStartBracket>(tokens[cursor]))
			throw new Error("[foreach] expects (",tokens[cursor]);

		++cursor;
		Object * array=getExpression(tokens,cursor);
		++cursor;
		tc=dynamic_cast<TControl *>(tokens[cursor]);
		if (!tc || tc->getId()!=Consts::IDENTIFIER_as)
			throw new Error("[foreach] expects as",tokens[cursor]);
		++cursor;

		TIdentifier * valueIdent=NULL;
		TIdentifier * keyIdent=NULL;
		if (!(valueIdent=dynamic_cast<TIdentifier *>(tokens[cursor])))
			throw new Error("[foreach] expects Identifier-1",tokens[cursor]);
		++cursor;

		if (Token::isA<TDelimiter>(tokens[cursor])) {
			++cursor;
			keyIdent=valueIdent;
			if (!(valueIdent=dynamic_cast<TIdentifier *>(tokens[cursor])))
				throw new Error("[foreach] expects Identifier-2",tokens[cursor]);
			++cursor;
		}
		if (!Token::isA<TEndBracket>(tokens[cursor]))
			throw new Error("[foreach] expects )",tokens[cursor]);
		++cursor;
		Object * action=getExpression(tokens,cursor);

		return new ForeachControl(array ,keyIdent?keyIdent->getId():0
								  ,valueIdent?valueIdent->getId():0,action);
	}

	/// try-catch-control
	else if(cId==Consts::IDENTIFIER_try) {
		Object * tryBlock=getExpression(tokens,cursor); // TODO should be a block
		++cursor;
		tc=dynamic_cast<TControl *>(tokens[cursor]);
		if (!tc || tc->getId()!=Consts::IDENTIFIER_catch)
			throw(new Error("[try-catch] expects catch",tokens[cursor]));
		++cursor;
		if (!Token::isA<TStartBracket>(tokens[cursor]))
			throw(new Error("[try-catch] expects (",tokens[cursor]));
		++cursor;
		TIdentifier * tIdent=NULL;

		identifierId varName=0;
		bool hasVarName=false;
		if ((tIdent=dynamic_cast<TIdentifier*>(tokens[cursor]))) {
			++cursor;
			varName=tIdent->getId();
			hasVarName=true;
		}
//        Object * Identifier=getExpression(tokens,cursor); //TODO should be an identifier


		if (!Token::isA<TEndBracket>(tokens[cursor])) {
			throw new Error("[try-catch] expects ([Identifier])",tokens[cursor]);
		}
		++cursor;
		Object * catchBlock=getExpression(tokens,cursor); // TODO should be a block
		if (!dynamic_cast<Block *>(catchBlock))
			throw new Error("[catch] expects Block {...} (Hint: {} is an empty Map!)",tokens[cursor]);


		/*if (!dynamic_cast<TEndCommand *>( tokens[cursor])) {
			throw(new Error("[do-while] expects ;",tokens[cursor]));
		}*/
		if (hasVarName)
			dynamic_cast<Block *>(catchBlock)->declareVar(varName);
		return new TryCatchControl(tryBlock,dynamic_cast<Block *>(catchBlock),varName);
	}
	/// continue-Control
	else if(cId==Consts::IDENTIFIER_continue) {
		return StateChangeControl::createContinueControl();
	}
	/// break-Control
	else if(cId==Consts::IDENTIFIER_break) {
		return StateChangeControl::createBreakControl();
	}
	/// return-Control
	else if(cId==Consts::IDENTIFIER_return) {
		return StateChangeControl::createReturnControl(getExpression(tokens,cursor));
	}
	/// exit-Control
	else if(cId==Consts::IDENTIFIER_exit) {
		return StateChangeControl::createExitControl(getExpression(tokens,cursor));
	}
	/// throw-Control
	else if(cId==Consts::IDENTIFIER_throw) {
		return StateChangeControl::createThrowControl(getExpression(tokens,cursor));
	}
	else{
		throw new Error(string("Parsing Unimplemented Control:")+tc->toString(),tokens[cursor]);
	}
}

/*!	getLValue
	\todo change string -> identifierId
*/
Parser::lValue_t Parser::getLValue(Tokenizer::tokenList & tokens,int from,int to,Object * & obj,
								identifierId & identifier,Object * &indexExpression) throw(Exception *) {

    /// Single Element: "a"
    if (to==from) {
        if (Token::isA<TIdentifier>(tokens[from])) {
            identifier=dynamic_cast<TIdentifier *>(tokens[from])->getId();
            obj=NULL;
            return LVALUE_MEMBER;
//        }else if (Identifier * i=dynamic_cast<Identifier *>(tokens[from])) { // $a
//            identifier=i->getId();
//            obj=NULL;
//            return LVALUE_MEMBER;
        } else {
            throw(new Error("LValue Error 1",tokens[from]));
        }
    }
    /// ".a"
    /// "a.b.c"
    if (Token::isA<TIdentifier>(tokens[to]) && Token::isA<TOperator>(tokens[to-1]) ) {
        if ( dynamic_cast<TOperator *>(tokens[to-1])->getOperator()->getString()==".") {
            obj=getExpression(tokens,from,to-2);
            identifier=dynamic_cast<TIdentifier *>(tokens[to])->getId();
            return LVALUE_MEMBER;
        }
    }
// !!!!!!!!!!!!!!!!!!!!!!!!!! \todo ..............................
    if(TObject * tObj=dynamic_cast<TObject*>(tokens[to])){
		/// ".'a'"
		/// "a.b.'c'"
		if (String * s=dynamic_cast<String *>(tObj->obj.get())) {
			TOperator * top=dynamic_cast<TOperator *>(tokens[to-1]);

			if (top && top->getOperator()->getString()==".") {
				obj=getExpression(tokens,from,to-2);
				identifier=stringToIdentifierId(s->toString());
				return LVALUE_MEMBER;
			}
		}
		/// ".$a"
		/// "a.b.$c"
		if (Identifier * i=dynamic_cast<Identifier *>(tObj->obj.get())) {
			TOperator * top=dynamic_cast<TOperator *>(tokens[to-1]);

			if (top && top->getOperator()->getString()==".") {
				obj=getExpression(tokens,from,to-2);
				identifier=i->getId();
				return LVALUE_MEMBER;
			}
		}
    }
    /// Index "a[1]"
    /// [a,b,c] //TODO!?
    if (Token::isA<TEndIndex>(tokens[to])) {

        int indexOpenPos=findCorrespondingBracket<TEndIndex,TStartIndex>(tokens,to,from,-1);
        /// a[1]
        if (indexOpenPos>from) {
            obj=getExpression(tokens,from,indexOpenPos-1);
            indexOpenPos++;
            indexExpression=getExpression(tokens,indexOpenPos,to-1);
            return LVALUE_INDEX;
        }
    }
    return LVALUE_NONE;
}


/**
 * int findExpression(Tokens, cursor)
 *
 * Returns the ending Position of the next Expression, starting at cursor.
 *
 */
int Parser::findExpression(Tokenizer::tokenList & tokens,int cursor) {
    Token * t;

    int level=0;
    int to=cursor-1;
    int lastIdentifier=-10;

    if (Token::isA<TEndScript>(tokens[cursor]))
		return 0;

    while (true) {
        to++;
        t=tokens[to];

		switch(t->getType()){
			case TStartBracket::TYPE_ID:{
				TStartBracket * sb=dynamic_cast<TStartBracket*>(t);
				if(sb->endBracketIndex>1){
					to=sb->endBracketIndex;
				}else {
					level++;
				}
				continue;
			}
			case TStartBlock::TYPE_ID:
			case TStartMap::TYPE_ID:
			case TStartIndex::TYPE_ID:{
				level++;
				continue;
			}
			case TEndBlock::TYPE_ID:
			case TEndBracket::TYPE_ID:
			case TEndMap::TYPE_ID:
			case TEndIndex::TYPE_ID:{
				level--;

				if (level<0) {
					to--;
					return to;
				}
				continue;
			}
			case TEndScript::TYPE_ID:{
				if (level==1)
					return to;

				std::cout << "\n!";//<<tokens[cursor]->toString();
				for(int i=cursor;i<to;++i)
					std::cout << " "<<tokens[i]->toString();
				throw new Error("Unexpected Ending.",tokens[cursor]);
			}
//
			default:{
			}
		}
        if (level>0)
            continue;
		switch(t->getType()){
			case TControl::TYPE_ID: {
				if (dynamic_cast<TControl *>(t)->getId()==Consts::IDENTIFIER_as) {
					to--;
					return to;
				}
				throw new Error("Expressions can't contain control statements.",t);
			}
			case TEndCommand::TYPE_ID:{
				return to;
			}
			case TDelimiter::TYPE_ID:
			case TMapDelimiter::TYPE_ID:
			case TColon::TYPE_ID:{
				to--;
				return to;
			}
			case TIdentifier::TYPE_ID:{
				if(lastIdentifier==to-1){
					to--;
					return to;
				}
				lastIdentifier=to;
				continue;
			}
			default:{
			}
		}
    }
    return to;
}
/**
 * e.g. (a, Number b, c=2+3)
 * Cursor is moved after the Parameter-List.
 */
UserFunction::parameterList_t * Parser::getFunctionParameters(Tokenizer::tokenList & tokens,int & cursor) throw(Exception *) {
    UserFunction::parameterList_t * params = new UserFunction::parameterList_t();

    if (!Token::isA<TStartBracket>(tokens[cursor])) {
        return params;
    }
    ++cursor;
    // fn (bla,blub,)
    bool first=true;

    while (true) { // foreach parameter
        if (first&&Token::isA<TEndBracket>(tokens[cursor])) {
            ++cursor;
            break;
        }
        first=false;

		/// Parameter::= Expression? Identifier ( ('=' Expression)? ',') | ('*'? ('=' Expression)? ')')
		int c=cursor;

		// find identifierName, its position, the default expression and identify a multiParam
		int idPos=-1;
		identifierId name=0;
		Object * defaultExpression=NULL;
		bool multiParam=false;
		while(true){
			Token * t=tokens[c];
			if(Token::isA<TIdentifier>(t)) {
				// this may not be the final identifier...
				name=dynamic_cast<TIdentifier*>(t)->getId();
				idPos=c;

				Token * tNext=tokens[c+1];
				// '*'?
				if(  Token::isA<TOperator>(tNext) && tNext->toString()=="*" ){
					multiParam=true;
					++c;
					tNext=tokens[c+1];
				}else{
					multiParam=false;
				}
				// ',' | ')'
				if( Token::isA<TEndBracket>(tNext)){
					break;
				}else if( Token::isA<TDelimiter>(tNext) ) {
					if(multiParam)
						throw new Error("[fn] Only the last parameter may be a multiparameter.",tokens[c]);
					break;
				}else if(  Token::isA<TOperator>(tNext) && tNext->toString()=="=" ){
					int defaultExpStart=c+2;
					int defaultExpTo=findExpression(tokens,defaultExpStart);
					defaultExpression=getExpression(tokens,defaultExpStart,defaultExpTo);
					if (defaultExpression==NULL) {
						throw new Error("[fn] SyntaxError in default parameter.",tokens[cursor]);
					}
					c=defaultExpTo;
					break;
				}
			}else if(Token::isA<TStartBracket>(t)){
				c=findCorrespondingBracket<TStartBracket,TEndBracket>(tokens,c);
			}else if(Token::isA<TStartIndex>(t)){
				c=findCorrespondingBracket<TStartIndex,TEndIndex>(tokens,c);
			}else if(Token::isA<TStartMap>(t)){
				c=findCorrespondingBracket<TStartMap,TEndMap>(tokens,c);
			}else if(Token::isA<TEndScript>(t) || Token::isA<TEndBracket>(t)){
				throw new Error("[fn] Error in parameter definition.",t);
			}
			++c;
		}

		// get the type expression
		Object * typeExp=NULL;
		if(	idPos>cursor ){
			int tmpCursor=cursor;
			typeExp=getExpression(tokens,tmpCursor,idPos-1);
		}

		// check if this is the last parameter
        bool lastParam=false;
        if(Token::isA<TEndBracket>(tokens[c+1])){
        	lastParam=true;
        }else if( ! Token::isA<TDelimiter>(tokens[c+1])){
        	throw new Error("[fn] SyntaxError.",tokens[c+1]);
        }

		// move cursor
		cursor=c+2;

        // create parameter
        UserFunction::Parameter * p=new UserFunction::Parameter(name,NULL,typeExp);
        if(multiParam)
			p->setMultiParam(true);
		if(defaultExpression!=NULL)
			p->setDefaultValueExpression(defaultExpression);
        params->push_back(p);
        if(lastParam){
			break;
        }
    }
    return params;
}

/*!	1,bla+2,(3*3)
	Cursor is moved at closing bracket ')'
*/
void Parser::getExpressionsInBrackets(Tokenizer::tokenList & tokens,int & cursor,std::vector<ObjRef> & expressions) throw (Exception *){
	Token * t=tokens[cursor];
	if(t->toString()!="(") {
		std::cout << " #"<<t->toString();
		throw new Error("Expression list error.",t);
	}
	++cursor;

	while (!Token::isA<TEndBracket>(tokens[cursor])) {
		if(Token::isA<TDelimiter>(tokens[cursor])){ // empty expression (1,,2)
			expressions.push_back(NULL);
			++cursor;
			continue;
		}
		expressions.push_back(getExpression(tokens,cursor));
		++cursor;
		if (Token::isA<TDelimiter>(tokens[cursor])){
			++cursor;
		}else if (!Token::isA<TEndBracket>(tokens[cursor])) {
			throw new Error("Expected )",tokens[cursor]);
		}
	}
}
