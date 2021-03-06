// test.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifdef ES_BUILD_TEST_APPLICATION
#include <cstdlib>
#include <iostream>
#include <string>

#include "../EScript/EScript.h"

#ifdef ES_DEBUG_MEMORY
#include "../EScript/Parser/Tokenizer.h"
#include "../EScript/Utils/Debug.h"
#endif

using namespace EScript;

// ---------------------------------------------------------
// test case for direct member access

//! A simple test class with some data members
struct TestObject{
	int m1;
	float m2;
	explicit TestObject(int _m1,float _m2) : m1(_m1),m2(_m2){}
	bool operator==(const TestObject&other)const {	return m1==other.m1 && m2==other.m2;}
};

static const identifierId ID_m1=stringToIdentifierId("m1");
static const identifierId ID_m2=stringToIdentifierId("m2");

//! A EScript-container for the simple test class
struct E_TestObject : public ReferenceObject<TestObject>{
	ES_PROVIDES_TYPE_NAME(TestObject)
public:
	static Type * getTypeObject() {
		static Type * type(new Type(Object::getTypeObject()));
		return type;
	}
	E_TestObject(int i=0,float f=0) : ReferenceObject<TestObject>(TestObject(i,f),getTypeObject()){}
	virtual ~E_TestObject(){}


	//! ---|> [Object]
	virtual Object * getAttribute(const identifierId id){
		if(id==ID_m1){
			return new NumberRef(ref().m1);
		}else if(id==ID_m2){
			return new NumberRef(ref().m2);
		}else{
			return Object::getAttribute(id);
		}
	}

	//! ---|> [Object]
	virtual bool assignAttribute(const identifierId id,ObjPtr val){
		if(id==ID_m1){
			ref().m1=val.toInt();
		}else if(id==ID_m2){
			ref().m2=val.toFloat();
		}else{
			return Object::assignAttribute(id,val);
		}
		return true;
	}
	//! (static)
	static void init(Namespace & ns){
		Type * typeObject=getTypeObject();
		declareConstant(&ns,getClassName(),typeObject);
		
		//! TestObject new TestObject([i [,j]])
		ESF_DECLARE(typeObject,"_constructor",0,2,new E_TestObject(parameter[0].toInt(),parameter[1].toFloat()))
		
		//! Number getM1()
		ESMF_DECLARE(typeObject,E_TestObject,"getM1",0,0,Number::create(self->ref().m1))

		//! Number getM2()
		ESMF_DECLARE(typeObject,E_TestObject,"getM2",0,0,Number::create(self->ref().m2))
		
	}
};

// ----------------------------------------------------------------------------

int main(int argc,char * argv[]) {

	EScript::init();

	// --- Init the TestObejct-Type
	E_TestObject::init(*EScript::getSGlobals()); 
	
#ifdef ES_DEBUG_MEMORY
	Tokenizer::identifyStaticToken(0); // init constants
	Debug::clearObjects();
#endif

	ERef<Runtime> rt(new Runtime());

	declareConstant(rt->getGlobals(),"args",Array::create(argc,argv));
	
	// --- Load and execute script
	std::string file= argc>1 ? argv[1] : "tests/test.escript";
	std::pair<bool,ObjRef> result = EScript::loadAndExecute(*rt.get(),file);

	// --- output result
	if (!result.second.isNull()) {
		std::cout << "\n\n --- "<<"\nResult: " << result.second.toString()<<"\n";
	}

	// --- cleanup
	result.second=NULL;
	rt=NULL;

#ifdef ES_DEBUG_MEMORY
	Debug::showObjects();
#endif
	return result.first ? EXIT_SUCCESS : EXIT_FAILURE;
}
#endif // ES_BUILD_TEST_APPLICATION
