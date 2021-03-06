// Parser.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef PARSER_H
#define PARSER_H

#include "Token.h"
#include "Tokenizer.h"

#include "../Objects/Exception.h"
#include "AST/UserFunctionExpr.h"
#include "../Utils/Logger.h"

#include <vector>

#if defined(_MSC_VER)
#pragma warning( disable : 4290 )
#endif

namespace EScript {
namespace AST{
class Block;
}

/*! [Parser] */
class Parser {
	public:

		/*! [ParserException] ---|> [Exception] ---|> [Object]    */
		class ParserException:public Exception {
			ES_PROVIDES_TYPE_NAME(ParserException)

			public:
				explicit ParserException(const std::string  & _msg,Token * token=nullptr):
					Exception(_msg,  (token==nullptr? -1 : token->getLine())) {
				}
				explicit ParserException(const std::string  & _msg,const _CountedRef<Token> & token):
					Exception(_msg,  (token.isNull() ? -1 : token->getLine())) {
				}
		};
		//-----------

		Parser(Logger * logger=nullptr);
		~Parser();

		ERef<AST::Block> parse(const CodeFragment & code);

		//! (internal)
		struct ParsingContext{
			Tokenizer::tokenList_t & tokens;
			AST::Block * rootBlock;
			std::deque<AST::Block*> blocks; // used as a stack
			CodeFragment code;
			ParsingContext(Tokenizer::tokenList_t & _tokens,const CodeFragment & _code ) : tokens(_tokens),rootBlock(nullptr),code(_code){}
		};

	private:

		_CountedRef<Logger> logger;
		void log(ParsingContext & ctxt,Logger::level_t messageLevel, const std::string & msg,const _CountedRef<Token> & token=nullptr)const;

		Tokenizer tokenizer;
		void pass_1(ParsingContext & ctxt);
		void pass_2(ParsingContext & ctxt, Tokenizer::tokenList_t  & enrichedTokens)const;
		EPtr<AST::ASTNode> readControl(ParsingContext & ctxt,int & cursor)const;
		EPtr<AST::ASTNode> readStatement(ParsingContext & ctxt,int & cursor)const;
		EPtr<AST::ASTNode> readExpression(ParsingContext & ctxt,int & cursor,int to=-1)const;
		EPtr<AST::ASTNode> readBinaryExpression(ParsingContext & ctxt,int & cursor,int to)const;
		AST::Block * readBlockExpression(ParsingContext & ctxt,int & cursor)const ;
		EPtr<AST::ASTNode> readMap(ParsingContext & ctxt,int & cursor)const;
		EPtr<AST::ASTNode> readFunctionDeclaration(ParsingContext & ctxt,int & cursor)const;
		void readFunctionParameters(AST::UserFunctionExpr::parameterList_t & params,ParsingContext & ctxt,int & cursor)const;
		void readExpressionsInBrackets(ParsingContext & ctxt,int & cursor,std::vector<ERef<AST::ASTNode>> & expressions)const;

		typedef std::vector<std::pair<StringId,int> > properties_t; //  (property's id, position of option bracket or -1)*
		void readProperties(ParsingContext & ctxt,int from,int to,properties_t & properties)const;

		enum lValue_t { LVALUE_NONE, LVALUE_INDEX, LVALUE_MEMBER};
		lValue_t getLValue(ParsingContext & ctxt,int from,int to,EPtr<AST::ASTNode> & obj,StringId & identifier,EPtr<AST::ASTNode> &indexExpression)const;
		int findExpression(ParsingContext & ctxt,int cursor)const;

		void throwError(ParsingContext & ctxt,const std::string & msg,Token * token=nullptr)const;
		void throwError(ParsingContext & ctxt,const std::string & msg,const _CountedRef<Token> & token)const	{	throwError(ctxt,msg,token.get());	}
};
}

#endif // PARSER_H
