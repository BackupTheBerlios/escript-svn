// Consts.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef ES_CONSTS_H_INCLUDED
#define ES_CONSTS_H_INCLUDED

#include "Utils/StringId.h"

namespace EScript{

/*! Common identifiers */
//	@{
struct Consts{
	static const StringId IDENTIFIER_attr_printableName;
	static const StringId IDENTIFIER_this;
	static const StringId IDENTIFIER_thisFn;
	static const StringId IDENTIFIER_fn_call;
	static const StringId IDENTIFIER_fn_constructor;
	static const StringId IDENTIFIER_fn_greater;
	static const StringId IDENTIFIER_fn_less;
	static const StringId IDENTIFIER_fn_equal;
	static const StringId IDENTIFIER_fn_identical;
	static const StringId IDENTIFIER_fn_getIterator;
	static const StringId IDENTIFIER_fn_get;
	static const StringId IDENTIFIER_fn_set;

	static const StringId IDENTIFIER_true;
	static const StringId IDENTIFIER_false;
	static const StringId IDENTIFIER_void;
	static const StringId IDENTIFIER_null;

	static const StringId IDENTIFIER_as;
	static const StringId IDENTIFIER_break;
	static const StringId IDENTIFIER_catch;
	static const StringId IDENTIFIER_continue;
	static const StringId IDENTIFIER_do;
	static const StringId IDENTIFIER_else;
	static const StringId IDENTIFIER_exit;
	static const StringId IDENTIFIER_for;
	static const StringId IDENTIFIER_foreach;
	static const StringId IDENTIFIER_if;
	static const StringId IDENTIFIER_namespace;
	static const StringId IDENTIFIER_return;
	static const StringId IDENTIFIER_throw;
	static const StringId IDENTIFIER_try;
	static const StringId IDENTIFIER_var;
	static const StringId IDENTIFIER_while;
	static const StringId IDENTIFIER_yield;

	static const StringId IDENTIFIER_LINE;

	static const StringId ANNOTATION_ATTR_const;
	static const StringId ANNOTATION_ATTR_init;
	static const StringId ANNOTATION_ATTR_member;
	static const StringId ANNOTATION_ATTR_override;
	static const StringId ANNOTATION_ATTR_private;
	static const StringId ANNOTATION_ATTR_public;
	static const StringId ANNOTATION_ATTR_type;

	static const StringId ANNOTATION_FN_super;
};
//	@}


}
#endif // ES_CONSTS_H_INCLUDED
