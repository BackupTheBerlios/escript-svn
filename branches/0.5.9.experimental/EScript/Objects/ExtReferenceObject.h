// ExtReferenceObject.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef EXT_REFERENCE_OBJECT_H
#define EXT_REFERENCE_OBJECT_H

#include "../Utils/AttributeContainer.h"
#include "ReferenceObject.h"
#include "Type.h"

namespace EScript {


namespace Policies{

/*! Policy for locating an ExtRefernceObject's attribute storage.
	Use this policy to directly store the AttributeContainer inside the ExtReferenceObject.
	Alternative implementations could e.g. store the attributeContainer as user data at the referenced object. */
class StoreAttrsInEObject_Policy{
		AttributeContainer attributeContainer;
	protected:
		/*!	(static) Returns a pointer to the object's attributeContainer. If @param create is 'false' and
			the object has no attributeContainer, the function returns NULL. If @param create is 'true' and no
			attributeContainer exists, a new one is created, so that always an valid container is returned. */
		static AttributeContainer * getAttributeContainer(StoreAttrsInEObject_Policy * obj,bool /*create*/){
			return &(obj->attributeContainer);
		}

		/*! (static) Should return true iff the object's Type's attributes are already initialized with the
			object's attributeContainer. This function is only called by the ExtReferenceObject's constructor.
			As for this specific policy, the attributeContainer has always just been created then, it can not already
			been initialized. */
		static bool areObjAttributesInitialized(StoreAttrsInEObject_Policy * /*obj*/){
			return false;
		}
};

}

/*! [ExtReferenceObject] ---|> [Object]
	A Ext(entable)ReferenceObject can be used as wrapper for user defined C++ objects that can be enriched by user
	defined attributes. For a description how the C++-object is handled and how the equalityComparator works, \see ReferenceObject.h
	The way the AttributeContainer is stored is controlled by the @tparam attributeProvider.
*/
template <typename _T,typename equalityComparator = _RefObjEqComparators::EqualContent, typename attributeProvider = Policies::StoreAttrsInEObject_Policy >
class ExtReferenceObject : public Object, private attributeProvider {
		ES_PROVIDES_TYPE_NAME(ExtReferenceObject)
	public:
		typedef ExtReferenceObject<_T,equalityComparator,attributeProvider> ExtReferenceObject_t;

		// ---
		ExtReferenceObject(const _T & _obj, Type * type=NULL) :
					Object(type), attributeProvider(), obj(_obj){

			if(type!=NULL && !areObjAttributesInitialized(this)){
				type->copyObjAttributesTo(this);
			}

		}
		virtual ~ExtReferenceObject()						{	}


		/*! ---|> [Object]
			Direct cloning of a ExtReferenceObject is forbidden; but you may override the clone function in the specific implementation */
		virtual ExtReferenceObject_t * clone()const {
			throw new Exception(std::string("Trying to clone unclonable object '")+this->toString()+"'");
		}
		/// ---|> [Object]
		virtual bool rt_isEqual(Runtime &,const ObjPtr o)	{	return equalityComparator::isEqual(this,o);	}


	// -----

	/*! @name Reference */
	//	@{
	public:
		inline const _T & ref() const 						{	return obj;	}
		inline _T & ref()  									{	return obj;	}

	private:
		_T obj;
	//	@}

	// -----

	/*! @name Attributes */
	//	@{
	public:
		using attributeProvider::getAttributeContainer;
		using Object::_initAttributes;
		using Object::_accessAttribute;
		using Object::setAttribute;

		/// ---|> [Object]
		virtual Attribute * _accessAttribute(const StringId id,bool localOnly){
			AttributeContainer * attrContainer = getAttributeContainer(this,false);
			Attribute * attr = attrContainer!=NULL ? attrContainer->accessAttribute(id) : NULL;
			return  ( attr!=NULL || localOnly || getType()==NULL) ? attr : getType()->findTypeAttribute(id);
		}

		/// ---|> [Object]
		virtual void _initAttributes(Runtime & rt){
			// if the type contains obj attributes, this object will surely also have some, so it is safe to init the attribute container.
			if(getType()!=NULL && getType()->getFlag(Type::FLAG_CONTAINS_OBJ_ATTRS) ){
				getAttributeContainer(this,true)->initAttributes(rt);
			}
		}

		/// ---|> [Object]
		virtual bool setAttribute(const StringId id,const Attribute & attr){
			getAttributeContainer(this,true)->setAttribute(id,attr);
			return true;
		}

		/// ---|> [Object]
		virtual void collectLocalAttributes(std::map<StringId,Object *> & attrs){
			AttributeContainer * attrContainer = getAttributeContainer(this,false);
			if(attrContainer!=NULL)
				attrContainer->collectAttributes(attrs);
		}
	// @}

};

}

#endif // EXT_REFERENCE_OBJECT_H
