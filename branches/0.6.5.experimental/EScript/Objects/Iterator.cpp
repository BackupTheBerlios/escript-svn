// Iterator.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "Iterator.h"
#include "../EScript.h"
#include "Values/Bool.h"

using namespace EScript;

//! initMembers
void Iterator::init(EScript::Namespace & globals) {
	Type * typeObject = getTypeObject();
	typeObject->allowUserInheritance(true);
	initPrintableName(typeObject,getClassName());

	declareConstant(&globals,getClassName(),typeObject);

	//! Bool Iterator.end()
	ESMF_DECLARE(typeObject,Iterator,"end",0,0,Bool::create(self->end()))

	//! Object Iterator.key()
	ESMF_DECLARE(typeObject,Iterator,"key",0,0,self->key())

	//! self Iterator.next()
	ESMF_DECLARE(typeObject,Iterator,"next",0,0,(self->next(),caller))

	//! self Iterator.reset()
	ESMF_DECLARE(typeObject,Iterator,"reset",0,0,(self->reset(),caller))

	//! Object Iterator.value()
	ESMF_DECLARE(typeObject,Iterator,"value",0,0,self->value())
}

//---