// Runtime.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "Runtime.h"
#include "FunctionCallContext.h"
#include "RuntimeInternals.h"

#include "../EScript.h"
#include "../Objects/Values/Void.h"
#include "../Objects/Exception.h"
#include "../Objects/Callables/Function.h"
#include "../Objects/Callables/UserFunction.h"
#include "../Objects/Callables/Delegate.h"
#include "../Objects/YieldIterator.h"
#include "../Utils/Logger.h"
#include "RtValue.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stack>

namespace EScript{

// ----------------------------------------------------------------------
// ---- Initialization

//!	initMembers
void Runtime::init(EScript::Namespace & globals) {
	Type * typeObject = getTypeObject();
	initPrintableName(typeObject,getClassName());

	declareConstant(&globals,getClassName(),typeObject);

	using EScript::create;
	declareConstant(typeObject,"LOG_DEBUG",				static_cast<int>(Logger::LOG_DEBUG));
	declareConstant(typeObject,"LOG_INFO",				static_cast<int>(Logger::LOG_INFO));
	declareConstant(typeObject,"LOG_PEDANTIC_WARNING",	static_cast<int>(Logger::LOG_PEDANTIC_WARNING));
	declareConstant(typeObject,"LOG_WARNING",			static_cast<int>(Logger::LOG_WARNING));
	declareConstant(typeObject,"LOG_ERROR",				static_cast<int>(Logger::LOG_ERROR));
	declareConstant(typeObject,"LOG_FATAL",				static_cast<int>(Logger::LOG_FATAL));

	//!	[ESMF] Number Runtime._getStackSize();
	ESF_DECLARE(typeObject,"_getStackSize",0,0, static_cast<uint32_t>(runtime.getStackSize()))

	//!	[ESMF] Number Runtime._getStackSizeLimit();
	ESF_DECLARE(typeObject,"_getStackSizeLimit",0,0, static_cast<uint32_t>(runtime._getStackSizeLimit()))

	//!	[ESMF] void Runtime._setStackSizeLimit(number);
	ESF_DECLARE(typeObject,"_setStackSizeLimit",1,1,
				(runtime._setStackSizeLimit(parameter[0].to<uint32_t>(runtime)),RtValue(nullptr)))

	//!	[ESMF] void Runtime.disableLogCounting( );
	ESF_DECLARE(typeObject,"disableLogCounting",0,1, (runtime.disableLogCounting(),RtValue(nullptr)))


	//!	[ESMF] void Runtime.enableLogCounting( );
	ESF_DECLARE(typeObject,"enableLogCounting",0,1, (runtime.enableLogCounting(),RtValue(nullptr)))

	//!	[ESMF] void Runtime.exception( [message] );
	ESF_DECLARE(typeObject,"exception",0,1, (runtime.setException(parameter[0].toString()),RtValue(nullptr)))

	//!	[ESMF] String Runtime.getLocalStackInfo();
	ESF_DECLARE(typeObject,"getLocalStackInfo",0,0, runtime.getLocalStackInfo())

	//!	[ESMF] Number Runtime.getLogCounter(Number);
	ESF_DECLARE(typeObject,"getLogCounter",1,1, runtime.getLogCounter(static_cast<Logger::level_t>(parameter[0].to<int>(runtime))))

	//!	[ESMF] Number Runtime.getLoggingLevel();
	ESF_DECLARE(typeObject,"getLoggingLevel",0,0, static_cast<int>(runtime.getLoggingLevel()))

	//!	[ESMF] String Runtime.getStackInfo();
	ESF_DECLARE(typeObject,"getStackInfo",0,0, runtime.getStackInfo())

	//!	[ESMF] void Runtime.log(Number,String);
	ESF_DECLARE(typeObject,"log",2,2,
				(runtime.log(static_cast<Logger::level_t>(parameter[0].to<int>(runtime)),parameter[1].toString()),RtValue(nullptr)))

	//!	[ESMF] void Runtime.resetLogCounter(Number);
	ESF_DECLARE(typeObject,"resetLogCounter",1,1,
				(runtime.resetLogCounter(static_cast<Logger::level_t>(parameter[0].to<int>(runtime))),RtValue(nullptr)))

	//!	[ESMF] void Runtime._setAddStackInfoToExceptions(bool);
	ESF_DECLARE(typeObject,"_setAddStackInfoToExceptions",1,1,
				(runtime.setAddStackInfoToExceptions(parameter[0].toBool()),RtValue(nullptr)))

	//!	[ESMF] void Runtime.setLoggingLevel(Number);
	ESF_DECLARE(typeObject,"setLoggingLevel",1,1,
				(runtime.setLoggingLevel(static_cast<Logger::level_t>(parameter[0].to<int>(runtime))),RtValue(nullptr)))

	//!	[ESMF] void Runtime.setTreatWarningsAsError(bool);
	ESF_DECLARE(typeObject,"setTreatWarningsAsError",1,1,
				(runtime.setTreatWarningsAsError(parameter[0].toBool()),RtValue(nullptr)))

	//!	[ESMF] void Runtime.warn([message]);
	ESF_DECLARE(typeObject,"warn",0,1, (runtime.warn(parameter[0].toString()),RtValue(nullptr)))

	// --- internals and experimental functions

	//! [ESF]  Object _callFunction(fun[,obj[,Array params]])
	ES_FUNCTION_DECLARE(typeObject,"_callFunction",1,3, {
		EPtr<Array> paramArr( (parameter.count()>2) ? assertType<Array>(runtime,parameter[2]) : nullptr );
		ParameterValues params(paramArr.isNotNull() ? paramArr->count() : 0);
		if(paramArr.isNotNull()){
			int i = 0;
			for(const auto & param : *paramArr.get()) {
				params.set(i++, param);
			}
		}
		return runtime.internals->startFunctionExecution(parameter[0],parameter[1],params);
	})

	//! [ESF]  Object _getCurrentCaller()
	ESF_DECLARE(typeObject,"_getCurrentCaller",0,0, runtime.getCallingObject().get() )
}

// ----------------------------------------------------------------------
// ---- Main

//! (ctor)
Runtime::Runtime() :
		ExtObject(Runtime::getTypeObject()), internals(new RuntimeInternals(*this)),
		logger(new LoggerGroup(Logger::LOG_WARNING)){

	logger->addLogger("coutLogger",new StdLogger(std::cout));
	//ctor
}

//! (dtor)
Runtime::~Runtime() {
	//dtor
}

bool Runtime::assertNormalState()const {
	return internals->checkNormalState();
}

bool Runtime::assignToAttribute(ObjPtr obj,StringId attrId,ObjPtr value){
	Attribute * attr = obj->_accessAttribute(attrId,false);
	if(attr == nullptr)
		return false;

	if(attr->getProperties()&Attribute::ASSIGNMENT_RELEVANT_BITS){
		if(attr->isConst()){
			setException("Cannot assign to const attribute.");
			return true;
		}else if(attr->isPrivate()){
			if( obj!=getCallingObject() ){
				setException("Cannot assign to private attribute.");
				return true;
			}
		}
		// the attribute is a reference -> do not set the new value object but assign the new value.
		if(attr->isReference()){
			attr->getValue()->_assignValue(value);
			return true;
		}
	}
	attr->setValue(value.get());
	return true;
}

bool Runtime::checkNormalState()const				{	return internals->checkNormalState();	}

ObjRef Runtime::createInstance(const EPtr<Type> & type,const ParameterValues & _params){
	if(!internals->checkNormalState())
		return nullptr;
	ParameterValues params(_params);
	RtValue callResult(std::move(internals->startInstanceCreation(type,params)));
	ObjRef resultObj;
	if(callResult.isFunctionCallContext()){ // user function?
		_CountedRef<FunctionCallContext> fcc = callResult._getFCC();
		resultObj = internals->executeFunctionCallContext(fcc);
	}else{
		resultObj = callResult.getObject(); // value should be an Object...
	}
	// error occured? throw an exception!
	if(internals->getState()==RuntimeInternals::STATE_EXCEPTION)
		throw internals->fetchAndClearException().detachAndDecrease();

	return resultObj;
}

ObjRef Runtime::executeFunction(const ObjPtr & fun,const ObjPtr & caller,const ParameterValues & _params){
	if(!internals->checkNormalState())
		return nullptr;
	ParameterValues params(_params);
	ObjRef resultObj;
	RtValue callResult(std::move(internals->startFunctionExecution(fun,caller,params)));
	if(callResult.isFunctionCallContext()){ // user function?
		_CountedRef<FunctionCallContext> fcc = callResult._getFCC();
		resultObj = internals->executeFunctionCallContext(fcc);
	}else{
		resultObj = callResult._toObject(); // the value should always be convertible to Object...
	}
	// error occured? throw an exception!
	if(internals->getState()==RuntimeInternals::STATE_EXCEPTION)
		throw internals->fetchAndClearException().detachAndDecrease();
	return resultObj;
}

ObjRef Runtime::fetchAndClearExitResult()			{	return internals->fetchAndClearExitResult();	}

ObjPtr Runtime::getCallingObject()const				{	return internals->getCallingObject();	}

std::string Runtime::getCurrentFile()const			{	return internals->getCurrentFile();	}

int Runtime::getCurrentLine()const					{	return internals->getCurrentLine();	}

Namespace * Runtime::getGlobals()const				{	return internals->getGlobals();	}

std::string Runtime::getStackInfo()					{	return internals->getStackInfo();	}

size_t Runtime::getStackSize()const					{	return internals->getStackSize();	}

size_t Runtime::_getStackSizeLimit()const			{	return internals->_getStackSizeLimit();	}

void Runtime::info(const std::string & s)			{	logger->info(s);	}

void Runtime::setAddStackInfoToExceptions(bool b)	{	internals->setAddStackInfoToExceptions(b);	}

void Runtime::_setExceptionState(ObjRef e)			{	internals->setExceptionState(std::move(e));	}

void Runtime::setException(const std::string & s)	{	internals->setException(s);	}

void Runtime::setException(Exception * e)			{	internals->setException(e);	}

void Runtime::_setExitState(ObjRef e)				{	internals->setExitState(e);	}

void Runtime::_setStackSizeLimit(const size_t s)	{	internals->_setStackSizeLimit(s);	}

void Runtime::setTreatWarningsAsError(bool b){
	if(b){ // --> disable coutLogger and add throwLogger
		Logger * coutLogger = logger->getLogger("coutLogger");
		if(coutLogger!=nullptr)
			coutLogger->setMinLevel(Logger::LOG_ERROR);

		//! ThrowLogger ---|> Logger
		class ThrowLogger : public Logger{
			Runtime & rt;
			virtual void doLog(level_t,const std::string & message){	rt.setException(message);	}
		public:
			ThrowLogger(Runtime & _rt) : Logger(LOG_PEDANTIC_WARNING,LOG_WARNING), rt(_rt){}
		};
		logger->addLogger("throwLogger",new ThrowLogger(*this));
	}else{
		Logger * coutLogger = logger->getLogger("coutLogger");
		if(coutLogger!=nullptr)
			coutLogger->setMinLevel(Logger::LOG_ALL);
		logger->removeLogger("throwLogger");
	}
}
void Runtime::throwException(const std::string & s,Object * obj){	internals->throwException(s,obj);	}

void Runtime::warn(const std::string & s)	{	internals->warn(s);	}

void Runtime::yieldNext(YieldIterator & yIt){
	_Ptr<FunctionCallContext> fcc = yIt.getFCC();
	if(fcc.isNull()){
		setException("Invalid YieldIterator");
		return;
	}
	ObjRef result( internals->executeFunctionCallContext( fcc ) );
	// error occured? throw an exception!
	if(internals->getState()==RuntimeInternals::STATE_EXCEPTION){
		throw internals->fetchAndClearException().detachAndDecrease();
	}
	YieldIterator * newYieldIterator = result.toType<YieldIterator>();

	// function exited with another yield? -> reuse the data for the current iterator
	if(newYieldIterator){
		yIt.setFCC( newYieldIterator->getFCC() );
		yIt.setValue( newYieldIterator->value() );
	} // function returned without yield? -> update and terminate the current iterator
	else{
		yIt.setFCC( nullptr );
		yIt.setValue( result.get() );
	}
}
std::string Runtime::getLocalStackInfo(){
	return internals->getLocalStackInfo();
}

// ----------------------------------------------------------------------------------
// ---- Logging

//! CountingLogger ---|> Logger
class CountingLogger : public Logger{
	std::map<level_t,uint32_t> counter;
	virtual void doLog(level_t l,const std::string & ){	++counter[l];	}
public:
	CountingLogger() : Logger(LOG_ALL,LOG_NONE){}
	~CountingLogger() {}
	uint32_t get(level_t l)const{
		std::map<level_t,uint32_t>::const_iterator it = counter.find(l);
		return it == counter.end() ? 0 : it->second;
	}
	void reset(level_t l)			{	counter[l] = 0;	}
};

void Runtime::enableLogCounting(){
	if(logger->getLogger("countingLogger")==nullptr)
		logger->addLogger("countingLogger",new CountingLogger);
}

void Runtime::disableLogCounting(){
	logger->removeLogger("countingLogger");
}

void Runtime::resetLogCounter(Logger::level_t level){
	CountingLogger * l = dynamic_cast<CountingLogger*>(logger->getLogger("countingLogger"));
	if(l!=nullptr)
		l->reset(level);
}

uint32_t Runtime::getLogCounter(Logger::level_t level)const{
	CountingLogger * l = dynamic_cast<CountingLogger*>(logger->getLogger("countingLogger"));
	return l==nullptr ? 0 : l->get(level);
}



// ------------------------------------------------------------------

}
