// Block.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef BLOCK_H
#define BLOCK_H

#include "../Object.h"
#include "Statement.h"

#include <deque>
#include <set>

namespace EScript {

/*! [Block]  ---|> [Object] */
class Block : public Object {
		ES_PROVIDES_TYPE_NAME(Block)
	public:
		typedef std::deque<Statement > statementList;
		typedef statementList::iterator statementCursor;
		typedef statementList::const_iterator cStatementCursor;
		typedef std::set<StringId>  declaredVariableMap_t;

		Block(int lineNr=-1);
		virtual ~Block();

		statementList & getStatements()                 {   return statements;  }
		void setFilename(StringId filename)  			{   filenameId=filename;  }
		std::string getFilename()const                  {   return filenameId.toString();    }
		int getLine()const								{	return line;	}


		/*! returns false if variable was already declared */
//		bool declareVar(const std::string & s)			{	return declareVar(EScript::stringToIdentifierId(s));	}
		bool declareVar(StringId id);
		const declaredVariableMap_t * getVars()const 	{ 	return vars;    }
		bool isLocalVar(StringId id)				{	return vars==NULL ? false : vars->count(id)>0;	}
		void addStatement(const Statement & s);
		bool hasLocalVars()const						{	return vars!=NULL && !vars->empty(); }

		/// ---|> [Object]
		virtual std::string toString()const ;
		virtual internalTypeId_t _getInternalTypeId()const {	return _TypeIds::TYPE_BLOCK; }
	private:
		StringId filenameId; // for debugging
		declaredVariableMap_t * vars;
		statementList statements;
		int line;


	public:
		static const int POS_DONT_HANDLE = -1;
		static const int POS_HANDLE_AND_LEAVE = -2;
		static const int POS_HANDLE_AND_RESTART = 0;

		int getNextPos()const			{	return statements.size();	}

		int getContinuePos()const		{	return continuePos;	}
		void setContinuePos(int pos)	{	continuePos=pos;	}
		int getBreakPos()const			{	return breakPos;	}
		void setBreakPos(int pos)		{	breakPos=pos;	}

		int getExceptionPos()const		{	return exceptionPos;	}
		void setExceptionPos(int pos)	{	exceptionPos=pos;	}

		int getJumpPosA()const			{	return jumpPosA;	}
		void setJumpPosA(int pos)		{	jumpPosA=pos;	}

	private:
		int continuePos;
		int breakPos;
		int exceptionPos;
		int jumpPosA;
		// exceptionPos

};
}

#endif // BLOCK_H
