#ifndef BOOL_H
#define BOOL_H

#include "../Object.h"
#include <string>

namespace EScript {

/*! [Bool] ---|> [Object]   */
class Bool : public Object {
		ES_PROVIDES_TYPE_NAME(Bool)
	public:
		static Type* typeObject;

		static void init(EScript::Namespace & globals);
        static Bool * create(bool value);
        static void release(Bool * b);

		// ---

		Bool(bool value,Type * type=NULL);
		virtual ~Bool();

		void setValue(bool b)				{	value=b;	}

		/// ---|> [Object]
		virtual Object * clone()const;
		virtual std::string toString()const;
		virtual bool toBool()const;
		virtual double toDouble()const;
		virtual bool rt_isEqual(Runtime & rt,const ObjPtr o);

	protected:
		bool value;
};
}

#endif // BOOL_H
