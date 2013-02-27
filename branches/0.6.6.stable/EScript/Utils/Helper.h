// Helper.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef ES_HELPER_H_INCLUDED
#define ES_HELPER_H_INCLUDED

#include "StringId.h"
#include "ObjArray.h"
//#include "../Objects/Callables/Function.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include <utility>

namespace EScript {
// Forward declarations.
class CodeFragment;
class ExtObject;
class Namespace;
class Object;
class Runtime;
class Type;
class Logger;
class StringData;
class UserFunction;
class RtValue;

typedef RtValue ( * _functionPtr)(Runtime & runtime,Object * caller, const ParameterValues & parameter);

// ---------------------------------------------------

//! @name Declaration
//@{

//! Add a type Function attribute to @p type with given name.
void declareFunction(Type * type, StringId nameId, _functionPtr fn);
void declareFunction(Type * type, const char * name, _functionPtr fn);
void declareFunction(Type * type, StringId nameId, int minParamCount, int maxParamCount, _functionPtr fn);
void declareFunction(Type * type, const char * name, int minParamCount, int maxParamCount, _functionPtr fn);
void declareConstant(Type * type, StringId nameId, const RtValue& value);
void declareConstant(Type * type, const char * name, const RtValue& value);

void declareFunction(Namespace * nameSpace, StringId nameId, _functionPtr fn);
void declareFunction(Namespace * nameSpace, const char * name, _functionPtr fn);
void declareFunction(Namespace * nameSpace, StringId nameId, int minParamCount, int maxParamCount, _functionPtr fn);
void declareFunction(Namespace * nameSpace, const char * name, int minParamCount, int maxParamCount, _functionPtr fn);
void declareConstant(Namespace * nameSpace, StringId nameId, const RtValue& value);
void declareConstant(Namespace * nameSpace, const char * name, const RtValue& value);

//! Adds a function as object attribute (it is copied to each instance)
void declareObjectFunction(Type * type, const char * name, _functionPtr fn);

void initPrintableName(Type * type, const std::string & printableName);
void initPrintableName(ExtObject * type, const std::string & printableName);

void markAttributeAsObjectAttribute(Type * type, StringId nameId);
void markAttributeAsObjectAttribute(Type * type, const char * name);

void copyAttributeAsAlias(Type * type, const char * originalName, const char * aliasName);
//@}

// ---------------------------------------------------

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
	if((paramCount < min && min >= 0) || ((paramCount > max) && max >= 0))
		assertParamCount_2(runtime, paramCount, min, max);
}
inline void assertParamCount(Runtime & runtime, const size_t paramCount, int min=-1, int max=-1) {
	const int paramCountI = static_cast<int>(paramCount);
	if((paramCountI < min && min >= 0) || ((paramCountI > max) && max >= 0))
		assertParamCount_2(runtime, paramCountI, min, max);
}

//! (internal) Non-inline part of @a assertType.
void assertType_throwError(Runtime & runtime, const ObjPtr & obj,const char * className);

/*!
 * Try to cast the given object to the specified type.
 * If the object is not of the appropriate type, a runtime error is thrown.
 */
template<class T> static T * assertType(Runtime & runtime, const ObjPtr & obj) {
	T * t = dynamic_cast<T *>(obj.get());
	if(t == nullptr) 
		assertType_throwError(runtime, obj, T::getClassName());
	return t;
}

ObjRef callMemberFunction(Runtime & rt, ObjPtr obj, StringId fnNameId, const ParameterValues & params);
ObjRef callFunction(Runtime & rt, Object * function, const ParameterValues & params);

/*! Compile and execute the given code and return the result.
	\note may throw 'std::exception' or 'Object *' on failure. */
ObjRef _eval(Runtime & runtime, const CodeFragment & code);

/*! Compile and execute the given code. If the code could be executed without an exception, (true,result) is returned;
	if an exception (of type Object*) occured (false,exception) is returned and the error message is sent to the runtime's logger.
	@param fileId Name used to identify the executed code in exception messages and stack traces; the default is '[inline]'
	@return (success, result)
*/
std::pair<bool, ObjRef> eval(Runtime & runtime, const StringData & code,const StringId & fileId = StringId());

//! @return (success, result)
std::pair<bool, ObjRef> executeStream(Runtime & runtime, std::istream & stream);

//! @return (success, result)
ObjRef _loadAndExecute(Runtime & runtime, const std::string & filename);

//! @return (success, result)
std::pair<bool, ObjRef> loadAndExecute(Runtime & runtime, const std::string & filename);

//! creates and throws an Exception objects.
void throwRuntimeException(const std::string & what);

//@}

}

#endif /* ES_HELPER_H_INCLUDED */
