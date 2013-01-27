// RtValue.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "RtValue.h"
#include "../Objects/Object.h"
#include "../Objects/Identifier.h"
#include "../Objects/Values/Bool.h"
#include "../Objects/Values/String.h"
#include "../Objects/Values/Number.h"
#include "../Objects/Values/String.h"
#include "../Objects/Values/Void.h"

namespace EScript{
std::string RtValue::toDbgString()const{
	switch(valueType){
		case VOID:
			return "void";
		case BOOL:
			return value.value_bool ? "true" : "false";
		case OBJECT_PTR:
			return value.value_obj->toDbgString();
		case UINT32:{
			std::ostringstream s;
			s<<value.value_uint32;
			return s.str();
		}
		case NUMBER:{
			std::ostringstream s;
			s<<value.value_number;
			return s.str();
		}
		case IDENTIFIER:
			return StringId::toString(value.value_indentifier);
		case LOCAL_STRING_IDX:{
			std::ostringstream s;
			s<<"[Local string #"<< value.value_localStringIndex <<"]";
			return s.str();
		}
		case UNDEFINED:
		default:
			return "";
	}
}

bool RtValue::toBool2()const{
	if(isObject()){
		return value.value_obj->toBool();
	}else if(isVoid() || isUndefined()){
		return false;
	}else{
		return true;
	}
}

Object * RtValue::_toObject()const{
	switch(valueType){
		case VOID:
			return Void::get();
		case BOOL:
			return Bool::create(value.value_bool);
		case OBJECT_PTR:
			return value.value_obj;
		case UINT32:
			return Number::create(value.value_uint32);
		case NUMBER:
			return Number::create(value.value_number);
		case IDENTIFIER:
			return Identifier::create(StringId( value.value_indentifier));
		case LOCAL_STRING_IDX:
			return String::create("[Local string]");
		case UNDEFINED:
		default:
			return nullptr;
	}
}


RtValue rtValue(const std::string & s){
	return rtValue(String::create(s));

}
}
