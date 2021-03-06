// LocalVarMap.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "LocalVarMap.h"

using namespace EScript;

//! (ctor)
LocalVarMap::LocalVarMap():parent(NULL),values(&m){
	//ctor
}

//! (dtor)
LocalVarMap::~LocalVarMap() {
	//dtor
}

void LocalVarMap::init(LocalVarMap *_parent) {
	this->parent=_parent;
	values = parent->getValues();
}

void LocalVarMap::init() {
	parent=NULL;
	values = &m;
}

void  LocalVarMap::reset(){
	while(!localVars.empty()){
		localVars.top()->pop();
		localVars.pop();
	}
	parent=NULL;
	values=&m;
	#ifdef ES_DEBUG_MEMORY
	// this clears the slots for unused variables, whichs makes dection of memory leaks easier,
	// but it introduces a huge performance penalty.
	m.clear();
	#endif
}

//! findAndUpdate
bool LocalVarMap::findAndUpdate(StringId varId,Object * val) {
	objMap_t::iterator i=getValues()->find(varId);
	if(i==getValues()->end())
		return false;
	objStack_t & s=i->second;
	if(s.empty())
		return false;
	s.top()=val;
	return true;
}
