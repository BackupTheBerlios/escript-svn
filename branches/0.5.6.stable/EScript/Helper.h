// Macros.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef ES_HELPER_H_INCLUDED
#define ES_HELPER_H_INCLUDED

#include "Utils/ObjArray.h"
#include "Utils/ObjRef.h"
#include "Objects/Exception.h"
#include "Objects/Function.h"
#include "Objects/Internals/Block.h"
#include "Utils/Hashing.h"
#include <cstddef>
#include <string>
#include <utility>

namespace EScript {
// Forward declarations.
class Namespace;
class Object;
class Runtime;
class Type;

//! @name Declaration
//@{

/*! Add a type Function attribute to @p type with given name. */
void declareFunction(Type * type, identifierId nameId, Function::functionPtr fn);
void declareFunction(Type * type, const char * name, Function::functionPtr fn);
void declareFunction(Type * type, identifierId nameId, int minParamCount, int maxParamCount, Function::functionPtr fn);
void declareFunction(Type * type, const char * name, int minParamCount, int maxParamCount, Function::functionPtr fn);
void declareConstant(Type * type, identifierId nameId, Object * value);
void declareConstant(Type * type, const char * name, Object * value);

void declareFunction(Namespace * nameSpace, identifierId nameId, Function::functionPtr fn);
void declareFunction(Namespace * nameSpace, const char * name, Function::functionPtr fn);
void declareFunction(Namespace * nameSpace, identifierId nameId, int minParamCount, int maxParamCount, Function::functionPtr fn);
void declareFunction(Namespace * nameSpace, const char * name, int minParamCount, int maxParamCount, Function::functionPtr fn);
void declareConstant(Namespace * nameSpace, identifierId nameId, Object * value);
void declareConstant(Namespace * nameSpace, const char * name, Object * value);
//@}

//! @name Runtime helper
//@{
//! (internal) Non-inline part of @a assertParamCount
void assertParamCount_2(Runtime & runtime, int paramCount, int min, int max);

/*!
 * Check if the number of given parameters is in the given range (min <= number <= max).
 * A range value of <0 accepts an arbitrary number of parameters.
 * If too many parameters are given, a warning message is shown.
 * If too few parameter are given, a runtime error is thrown.
 */
inline void assertParamCount(Runtime & runtime, const ParameterValues & params, int min=-1, int max=-1) {
	const int paramCount = static_cast<int>(params.count());
	if((paramCount < min && min >= 0) || ((paramCount > max) && max >= 0)) {
		assertParamCount_2(runtime, paramCount, min, max);
	}
}

//! (internal) Non-inline part of @a assertType.
void assertType_throwError(Runtime & runtime, const ObjPtr & obj);

/*!
 * Try to cast the given object to the specified type.
 * If the object is not of the appropriate type, a runtime error is thrown.
 */
template<class T> static T * assertType(Runtime & runtime, const ObjPtr & obj) {
	T * t = dynamic_cast<T *>(obj.get());
	if (t == NULL) {
		assertType_throwError(runtime, obj);
	}
	return t;
}

Object * callMemberFunction(Runtime & rt, ObjPtr obj, identifierId fnNameId, const ParameterValues & params);
Object * callMemberFunction(Runtime & rt, ObjPtr obj, const std::string & fnName, const ParameterValues & params);
Object * callFunction(Runtime & rt, Object * function, const ParameterValues & params);

void out(Object * obj);

Block * loadScriptFile(const std::string & filename) throw (Exception *);

//! @return (success, result)
std::pair<bool, ObjRef> loadAndExecute(Runtime & runtime, const std::string & filename);

//@}

}

#endif /* ES_HELPER_H_INCLUDED */
