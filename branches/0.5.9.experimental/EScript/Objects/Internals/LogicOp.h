// LogicOp.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef LOGICOP_H
#define LOGICOP_H

#include "../Object.h"
#include <string>

namespace EScript {

/*! [LogicOp]  ---|> [Object]   */
class LogicOp : public Object {
		ES_PROVIDES_TYPE_NAME(LogicOp)
	public:
		enum opType_T{
			OR,AND,NOT
		};

		LogicOp(Object * left,Object * right,opType_T op);
		virtual ~LogicOp();

		Object * getLeft()const  		{   return leftRef.get();   }
		Object * getRight()const 		{   return rightRef.get();  }
		opType_T getOperator()const   	{   return op;  }

		/// ---|> [Object]
		virtual std::string toString()const;
		virtual internalTypeId_t _getInternalTypeId()const {	return _TypeIds::TYPE_LOGIC_OP; }

	private:
		ObjRef leftRef;
		ObjRef rightRef;
		opType_T op;
};
}

#endif // LOGICOP_H
