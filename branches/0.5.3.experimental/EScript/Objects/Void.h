// Void.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef VOIDVALUE_H
#define VOIDVALUE_H

#include "Type.h"

namespace EScript {
class Namespace;

/*! [Void] ---|> [Type] ---|> [Object]   */
class Void : public Type {
		ES_PROVIDES_TYPE_NAME(Void)
	public:
		static Void * get();

		/// ---|> [Object]
		virtual Object * clone()const;
		virtual bool toBool()const;
		virtual bool rt_isEqual(Runtime & rt,const ObjPtr o);

	protected:
		virtual ~Void();
		Void();
};
}

#endif // VOIDVALUE_H
