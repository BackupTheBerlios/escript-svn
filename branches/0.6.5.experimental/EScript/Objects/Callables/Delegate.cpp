// Delegate.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "Delegate.h"

#include "../../EScript.h"

namespace EScript{

//----
static std::stack<Delegate *> pool;

Delegate * Delegate::create(ObjPtr object,ObjPtr function){
	#ifdef ES_DEBUG_MEMORY
	return new Delegate(object,function);
	#endif
	if(pool.empty()){
		for(int i = 0;i<32;++i){
			pool.push(new Delegate(nullptr,nullptr));
		}
		return create(object,function);
	}else{
		Delegate * o = pool.top();
		pool.pop();
		o->myObjectRef = object;
		o->functionRef = function;
//        std::cout << ".";
		return o;
	}

}
void Delegate::release(Delegate * o){
	#ifdef ES_DEBUG_MEMORY
	delete o;
	return;
	#endif
	pool.push(o);
}

//! initMembers
void Delegate::init(EScript::Namespace & globals) {
	Type * typeObject = getTypeObject();
	initPrintableName(typeObject,getClassName());

	declareConstant(&globals,getClassName(),typeObject);

	//!	[ESMF] Delegate new Delegate(object,function)
	ESF_DECLARE(typeObject,"_constructor",2,2,	new Delegate(parameter[0],parameter[1]))

	//!	[ESMF] Object Delegate.getObject()
	ESMF_DECLARE(typeObject,Delegate,"getObject",0,0,self->getObject())

	//!	[ESMF] Object Delegate.getFunction()
	ESMF_DECLARE(typeObject,Delegate,"getFunction",0,0,self->getFunction())
}

//---

//! (ctor)
Delegate::Delegate(ObjPtr object,ObjPtr function):
		Object(getTypeObject()),myObjectRef(object.get()),functionRef(function.get()) {
	//ctor
}


//! ---|> [Object]
Delegate * Delegate::clone() const{
	return new Delegate(getObject(),getFunction());
}

//! ---|> [Object]
bool Delegate::rt_isEqual(Runtime &,const ObjPtr & o){
	Delegate * d = o.toType<Delegate>();
	return (d!=nullptr) &&   d->getObject()==getObject() && d->getFunction()==getFunction();
}

//! ---|> [Object]
std::string Delegate::toDbgString()const {
	return std::string('('+myObjectRef.toString("?")+"->"+(functionRef.isNull() ? "?" : functionRef->toDbgString())+')');
}
}
