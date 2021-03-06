// ReferenceObject.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef REFERENCE_OBJECT_H
#define REFERENCE_OBJECT_H

#include "Object.h"
#include "Exception.h"

namespace EScript {

//! (internal) Collection of comparators used for comparing ReferenceObjects.
namespace _RefObjEqComparators{ //! \todo --> Policies_RefObjEqComparators
	struct EqualContent{
		template <typename ReferenceObject_t>
		static inline bool isEqual(ReferenceObject_t * a,const ObjPtr b)	{
			ReferenceObject_t * other = b.toType<ReferenceObject_t >();
			return other && a->ref() == other->ref();
		}
	};

	struct SameEObjects{
		static inline bool isEqual( Object * a,const ObjPtr b)	{	return a==b.get();	}
	};
}

/*! [ReferenceObject] ---|> [Object]
	A ReferenceObject can be used as wrapper for user defined C++ objects. The encapsulated
	data can be an object, a pointer or a smart reference and is defined by the first template parameter.

	The second template parameter defines how two instances are compared during an test for equality.
	If the default value '_RefObjEqComparators::EqualContent' is used, the two referenced values are
	compared using their '=='-operator (which has to be defined for the values's type).
	If '_RefObjEqComparators::SameEObjects' is used, the pointers of the two compared ReferenceObject are used
	for equality testing. The latter can be used if the values is an object (and not a reference) and
	the this object does not define a '==' operator.
*/
template <typename _T,typename equalityComparator = _RefObjEqComparators::EqualContent >
class ReferenceObject : public Object {
		ES_PROVIDES_TYPE_NAME(ReferenceObject)
	public:
		typedef ReferenceObject<_T,equalityComparator> ReferenceObject_t;

		// ---
		ReferenceObject(const _T & _obj, Type * type=NULL):
				Object(type),obj(_obj)						{	}
		virtual ~ReferenceObject()							{	}

		inline const _T & ref() const 						{	return obj;	}
		inline _T & ref()  									{	return obj;	}

		/*! ---|> [Object]
			Direct cloning of a ReferenceObject is forbidden; but you may override the clone function in the specific implementation */
		virtual ReferenceObject_t * clone()const {
			throw new Exception(std::string("Trying to clone unclonable object '")+this->toString()+"'");

		}
		/// ---|> [Object]
		virtual bool rt_isEqual(Runtime &,const ObjPtr o)	{	return equalityComparator::isEqual(this,o);	}
	private:
		_T obj;
};

}

#endif // REFERENCE_OBJECT_H
