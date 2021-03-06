// Number.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef NUMBER_H
#define NUMBER_H

#include "../Object.h"
#include <stack>
#include <string>

#include <limits>

// assure M_PI is defined in VC (necessary for VC)
#define _USE_MATH_DEFINES
#include <cmath>

namespace EScript {

/*! [Number] ---|> [Object] */
class Number : public Object {
		ES_PROVIDES_TYPE_NAME(Number)
	public:
		static Type* typeObject;
		static void init(EScript::Namespace & globals);

		// ---
		static Number * create(double value);
		static Number * create(double value,Type * type);
		static void release(Number * n);

		// ---
		/**
		 * Test if the two parameters are essentially equal as defined in
		 * the given reference by Knuth.
		 *
		 * @param u First floating point parameter.
		 * @param v Second floating point parameter.
		 * @return @c true if both floating point values are essentially
		 * equal, @c false otherwise.
		 * @see Donald E. Knuth: The art of computer programming.
		 * Volume II: Seminumerical algorithms. Addison-Wesley, 1969.
		 * Page 128, Equation (24).
		 */
		inline static bool matches(const float u, const float v) {
			return std::abs(v - u) <= std::numeric_limits<float>::epsilon() * std::min(std::abs(u), std::abs(v));
		}

		Number(double value,Type * type=NULL,bool isReference=false);
		virtual ~Number();

		/**
		 * Convert the number to a string configurable string representation.
		 *
		 * @param precision Number of digits after the decimal point.
		 * @param scientific Switch for scientific or fixed-point notation.
		 * @param width Number of digits for padding.
		 * @param fill Character to use for padding.
		 * @return Formatted string representation of the number.
		 */
		std::string format(std::streamsize precision = 3, bool scientific = true, std::streamsize width = 0, char fill = '0') const;

		//! Floating point symmetric modulo operation
		double modulo(const double m)const;

		/// ---o
		virtual double getValue()const;
		virtual void setValue(double _value);

		/// ---|> [Object]
		virtual Object * clone()const;

		virtual std::string toString()const;
		virtual double toDouble()const;
		virtual bool toBool()const;
		virtual bool rt_isEqual(Runtime & rt,const ObjPtr o);
		virtual internalTypeId_t _getInternalTypeId()const 	{	return _TypeIds::TYPE_NUMBER;	}

	protected:
		union{
			void * valuePtr;
			double doubleValue;
		};

		static std::stack<Number *> numberPool;
};
}

#endif // NUMBER_H
