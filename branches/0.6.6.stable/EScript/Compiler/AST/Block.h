// Block.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef ES_BLOCK_EXPR_H
#define ES_BLOCK_EXPR_H

#include "ASTNode.h"

#include <deque>
#include <set>

namespace EScript {
namespace AST {

//! [Block]  ---|> [ASTNode]
class Block : public ASTNode {
		ES_PROVIDES_TYPE_NAME(Block)
	public:
		typedef std::deque<ref_t> statementList_t;
		typedef statementList_t::iterator statementCursor;
		typedef statementList_t::const_iterator cStatementCursor;
		typedef std::set<StringId>  declaredVariableMap_t;

		static Block * createBlockExpression(int line=-1){
			return new Block(TYPE_BLOCK_EXPRESSION,true,line);	
		}
		static Block * createBlockStatement(int line=-1){
			return new Block(TYPE_BLOCK_STATEMENT,false,line);
		}

		virtual ~Block() {}

		statementList_t & getStatements()				{	return statements;	}
		const statementList_t & getStatements()const	{	return statements;	}


		//! returns false if variable was already declared
		bool declareVar(StringId id)					{	return vars.insert(id).second;	}
		const declaredVariableMap_t & getVars()const	{	return vars;	}
		bool isLocalVar(StringId id)					{	return vars.count(id)>0;	}
		void addStatement(ptr_t s)						{	statements.push_back(s);	}
		bool hasLocalVars()const						{	return !vars.empty(); }
		size_t getNumLocalVars()const					{	return vars.size(); }

		void convertToStatement(){	convert(TYPE_BLOCK_STATEMENT,false);	}
		void convertToExpression(){	convert(TYPE_BLOCK_EXPRESSION,true);	}
	private:
		Block(nodeType_t t,bool expr,int l) : ASTNode(t,expr,l ){}
		declaredVariableMap_t vars;
		statementList_t statements;
};
}
}

#endif // ES_BLOCK_EXPR_H
