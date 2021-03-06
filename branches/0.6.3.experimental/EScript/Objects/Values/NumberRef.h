// NumberRef.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef NUMBERREF_H
#define NUMBERREF_H

#include "Number.h"

namespace EScript {

/*! [NumberRef] ---|> [Number] ---|> [Object]   */
class NumberRef : public Number {
		ES_PROVIDES_TYPE_NAME(NumberRef)
	public:
		static Type * getTypeObject();
		static void init(EScript::Namespace & globals);

		// ---
		explicit NumberRef(double & valueRef,Type * type=nullptr);
		explicit NumberRef(float & valueRef,Type * type=nullptr);
		explicit NumberRef(int & valueRef,Type * type=nullptr);
		explicit NumberRef(unsigned int & valueRef,Type * type=nullptr);
		explicit NumberRef(char & valueRef,Type * type=nullptr);
		explicit NumberRef(unsigned char & valueRef,Type * type=nullptr);
		explicit NumberRef(long & valueRef,Type * type=nullptr);
		virtual ~NumberRef();

		/// ---|> Number
		virtual double  getValue()const;
		virtual void setValue(double _value);

		/// ---|> [Object]
		virtual Object * getRefOrCopy();

		/// ---|> [Object]
		virtual void _assignValue(ObjPtr value);

	private:

		enum ValueType {
			VT_DOUBLE = 0,
			VT_FLOAT = 1,
			VT_INT = 2,
			VT_UINT = 3,
			VT_CHAR = 4,
			VT_UCHAR = 5,
			VT_LONG = 6
		};
		ValueType valueType;
};

}

#endif // NUMBERREF_H
