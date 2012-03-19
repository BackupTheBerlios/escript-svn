// UserFunction.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "UserFunction.h"
#include "../AST/BlockStatement.h"
#include "../../EScript.h"
#include "../../Parser/CompilerContext.h"
#include "../../Parser/Compiler.h"
#include <sstream>

using namespace EScript;

//! (ctor) UserFunction::Parameter
UserFunction::Parameter::Parameter(const StringId & _name,Object * defaultValueExpression/*=NULL*/,Object * type/*=NULL*/):
	name(_name),defaultValueExpressionRef(defaultValueExpression),typeRef(type),multiParam(false){
}

//! (dtor) UserFunction::Parameter
UserFunction::Parameter::~Parameter(){
}

std::string UserFunction::Parameter::toString()const{
	std::string s=typeRef.toString()+' '+name.toString();
	if(!defaultValueExpressionRef.isNull())
		s+='='+defaultValueExpressionRef->toDbgString();
	if(multiParam)
		s+='*';
	return s;
}
UserFunction::Parameter * UserFunction::Parameter::clone()const{
	Parameter * p = new Parameter( name,
								defaultValueExpressionRef.isNotNull() ? defaultValueExpressionRef->getRefOrCopy() : NULL,
								typeRef.isNotNull() ? typeRef->getRefOrCopy() : NULL);
	p->setMultiParam(isMultiParam());
	return p;
}
// ------------------------------------------------------------

//! (static)
Type * UserFunction::getTypeObject()	{
	static Type * typeObject=new Type(ExtObject::getTypeObject());
	return typeObject;
}

//! (static) initMembers
void UserFunction::init(EScript::Namespace & globals) {
	// [UserFunction] ---|> [ExtObject] ---|> [Object]
	Type * t=getTypeObject();
	declareConstant(&globals,getClassName(),t);

	//! [ESMF] String UserFunction.getFilename()
	ESMF_DECLARE(t,UserFunction,"getFilename",0,0,String::create(self->getFilename()))

	//! [ESMF] String UserFunction.getCode()
	ESMF_DECLARE(t,UserFunction,"getCode",0,0,String::create(self->getCode()))

	//! [ESMF] Number|false UserFunction.getMaxParamCount()
	ES_MFUNCTION_DECLARE(t,UserFunction,"getMaxParamCount",0,0,{
		if(self->getMaxParamCount()<0 )
			return Bool::create(false);
		return Number::create(self->getMaxParamCount());
	})

	//! [ESMF] Number UserFunction.getMinParamCount()
	ESMF_DECLARE(t,UserFunction,"getMinParamCount",0,0, Number::create(self->getMinParamCount()))

	//! [ESMF] String UserFunction._asm()
	ESMF_DECLARE(t,UserFunction,"_asm",0,0, String::create(self->getInstructions().toString()))

	//! [ESMF] String UserFunction._doCall()
	ESMF_DECLARE(t,UserFunction,"_doCall",0,0, runtime.executeUserFunction(self))

}

//! (ctor)
UserFunction::UserFunction(parameterList_t * _params,AST::BlockStatement * block):
		ExtObject(getTypeObject()), params(_params),posInFile(0),codeLen(0) {

	setBlock(block);
	
	initInstructions();
	//ctor
}
//! (ctor)
UserFunction::UserFunction(parameterList_t * _params,AST::BlockStatement * block,const std::vector<ObjRef> & _sConstrExpressions):
		ExtObject(getTypeObject()), params(_params),sConstrExpressions(_sConstrExpressions.begin(),_sConstrExpressions.end()),posInFile(0),codeLen(0) {

	setBlock(block);
	initInstructions();
	//ctor
}

//! (dtor)
UserFunction::~UserFunction() {
	for (parameterList_t::iterator it=params->begin();it!=params->end();++it) {
		delete (*it);
	}
	delete params;
	//dtor
}

void UserFunction::initInstructions(){
	instructions.declareLocalVariable(Consts::IDENTIFIER_this); // $0 : LOCAL_VAR_INDEX_this
	instructions.declareLocalVariable(Consts::IDENTIFIER_thisFn); // $1 : LOCAL_VAR_INDEX_thisFn
	instructions.declareLocalVariable(Consts::IDENTIFIER_internalResult); // $2 : LOCAL_VAR_INDEX_internalResult
	for(parameterList_t::const_iterator it = params->begin();it!=params->end();++it){
		instructions.declareLocalVariable( (**it).getName() );
	}
	//! \todo init parameters

}


void UserFunction::setBlock(AST::BlockStatement * _block){
	blockRef=_block;
}

//! ---|> [Object]
UserFunction * UserFunction::clone()const{
	parameterList_t * params2 = new parameterList_t;
	for(parameterList_t::const_iterator it = params->begin();it!=params->end();++it){
		params2->push_back( (*it)->clone());
	}

	UserFunction * f = new UserFunction( params2,blockRef.get(),sConstrExpressions );
	f->cloneAttributesFrom( this );
	f->setCodeString(fileString,posInFile,codeLen);
	return f;
}

//! ---|> [Object]
std::string UserFunction::toDbgString()const {
	std::ostringstream sprinter;
	sprinter << "fn(";
	int nr=0;
	for (parameterList_t::const_iterator it=params->begin();it!=params->end();++it) {
		if (nr++>0) sprinter<< ',';
		if ( (*it) ) sprinter<<(*it)->toString();
	}
	sprinter << "){...} "<<'['+getFilename()<<':'<<getLine()<<']';
	return sprinter.str();
}


std::string UserFunction::getFilename()const {
	return blockRef.isNull() ? "" : blockRef->getFilename();
}


int UserFunction::getLine()const	{
	return blockRef.isNull() ? -1 : blockRef->getLine();
}

void UserFunction::setCodeString(const EPtr<String> & _fileString,size_t _begin,size_t _codeLen){
	fileString = _fileString.get();
	posInFile = _begin;
	codeLen = _codeLen;
}

std::string UserFunction::getCode()const{
	return fileString->toString().substr(posInFile,codeLen);
}

int UserFunction::getMaxParamCount()const{
	if(params->empty()){
		return 0;
	}else if(params->back()->isMultiParam()){
		return -1;
	}else
		return params->size();
}

int UserFunction::getMinParamCount()const{
	int i=0;
	for (parameterList_t::const_iterator it=params->begin();it!=params->end();++it) {
		if( (*it)->isMultiParam() || (*it)->getDefaultValueExpression() !=NULL )
			break;
		++i;
	}
	return i;
}

