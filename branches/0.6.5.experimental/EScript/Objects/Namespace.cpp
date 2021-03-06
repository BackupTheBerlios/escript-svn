// Namespace.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "Namespace.h"

#include "../EScript.h"

using namespace EScript;
//---

//! initMembers
void Namespace::init(EScript::Namespace & globals) {
	// [Namespace] ---|> [ExtObject] ---|> [Object]
	Type * typeObject = getTypeObject();
	initPrintableName(typeObject,getClassName());

	declareConstant(&globals,getClassName(),typeObject);

	//! [ESMF] Namespace new Namespace
	ESF_DECLARE(typeObject,"_constructor",0,0, new Namespace)
}

//---

//! ---|> [Object]
Namespace * Namespace::clone() const{
	Namespace * c = new Namespace(getType());
	c->cloneAttributesFrom(this);
	return c;
}
