// MathLib.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "MathLib.h"
#define _USE_MATH_DEFINES
#include <cmath>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#define M_PI_2		1.57079632679489661923
#endif

#include "../EScript/EScript.h"
#include <ctime>
#include <random>

namespace EScript{
namespace MathLib{


// ---------------------------------------------------------

//! EWrapper for C++ random number engine
class E_RandomNumberGenerator : public ReferenceObject<std::mt19937> {
	ES_PROVIDES_TYPE_NAME(RandomNumberGenerator)
	public:
		//! (static)
		static Type * getTypeObject() {
			static Type * typeObject = new Type(Object::getTypeObject());
			return typeObject;
		}
		static void init(EScript::Namespace & globals);

		//! (ctor)
		E_RandomNumberGenerator(uint32_t seed) :
			ReferenceObject_t(std::mt19937(seed), getTypeObject()) {
		}
		
		E_RandomNumberGenerator(const std::mt19937 & engine) :
			ReferenceObject_t(engine, getTypeObject()) {
		}

		//! (dtor)
		virtual ~E_RandomNumberGenerator() {
		}

		//! ---|> Object
		virtual E_RandomNumberGenerator * clone() const {
			return new E_RandomNumberGenerator(ref());
		}
};


// ---------------------------------------------------------

//! (static) MathLib init
void init(EScript::Namespace * globals) {
	Namespace * lib = new Namespace;
	declareConstant(globals,"Math",lib);

	declareConstant(lib,"PI",	M_PI);
	declareConstant(lib,"PI_2",	M_PI_2);


	//! Number Math.atan2(a,b)
	ESF_DECLARE(lib, "atan2", 2, 2,
				std::atan2(parameter[0].to<double>(runtime), parameter[1].to<double>(runtime)))


	// init E_RandomNumberGenerator
	E_RandomNumberGenerator::init(*lib);

	// init global E_RandomNumberGenerator-Object
	declareConstant(globals, "Rand", new E_RandomNumberGenerator(static_cast<unsigned int>(std::time(nullptr))));

	// ------
}

// ---------------------------------------------------------------

//! (static) init members for E_RandomNumberGenerator
void E_RandomNumberGenerator::init(EScript::Namespace & lib) {
	// E_Rand ---|> [Object]
	Type * typeObject = getTypeObject();
	declareConstant(&lib, getClassName(), typeObject);

	//! [ESF] new RandomNumberGenerator( [seed] )
	ESF_DECLARE(typeObject, "_constructor", 0, 1, 
				new E_RandomNumberGenerator(parameter[0].toInt(0)))

	//! [ESMF] [0, 1] RandomNumberGenerator.bernoulli(p)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "bernoulli", 1, 1,
				 std::bernoulli_distribution(parameter[0].to<double>(runtime))(**self) ? 1 : 0)

	//! [ESMF] Number RandomNumberGenerator.binomial(n,p)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "binomial", 2, 2,
				 std::binomial_distribution<int>(parameter[0].to<int>(runtime), parameter[1].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.categorical(Array weights)
	ES_MFUNCTION_DECLARE(typeObject, E_RandomNumberGenerator, "categorical", 1, 1, {
		Array * array = assertType<EScript::Array>(runtime, parameter[0]);
		std::vector<double> weights;
		weights.reserve(array->size());
		for(const auto & element : *array) {
			weights.push_back(element->toDouble());
		}
		return std::discrete_distribution<int>(weights.begin(), weights.end())(**self);
	})
	
	//! [ESMF] Number RandomNumberGenerator.chisquare(n)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "chisquare", 1, 1,
				 std::chi_squared_distribution<double>(parameter[0].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.equilikely(a,b)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "equilikely", 2, 2,
				 std::uniform_int_distribution<int>(parameter[0].to<int>(runtime), parameter[1].to<int>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.exponential(m)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "exponential", 1, 1,
				 std::exponential_distribution<double>(parameter[0].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.geometric(p)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "geometric", 1, 1,
				 std::geometric_distribution<int>(parameter[0].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.lognormal(a,b)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "lognormal", 2, 2,
				 std::lognormal_distribution<double>(parameter[0].to<double>(runtime), parameter[1].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.normal(m,s)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "normal", 2, 2,
				 std::normal_distribution<double>(parameter[0].to<double>(runtime), parameter[1].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.pascal(n,p)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "pascal", 2, 2,
				 std::negative_binomial_distribution<int>(parameter[0].to<int>(runtime), parameter[1].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.poisson(m)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "poisson", 1, 1,
				 std::poisson_distribution<int>(parameter[0].to<double>(runtime))(**self))

	//! [ESMF] self RandomNumberGenerator.setSeed(Number)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "setSeed", 1, 1,
				 ((**self).seed(parameter[0].to<int>(runtime)), self))

	//! [ESMF] Number RandomNumberGenerator.student(n)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "student", 1, 1,
				 std::student_t_distribution<double>(parameter[0].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.uniform(a,b)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "uniform", 2, 2,
				 std::uniform_real_distribution<double>(parameter[0].to<double>(runtime), parameter[1].to<double>(runtime))(**self))

	//! [ESMF] Number RandomNumberGenerator.weibull(shape, scale)
	ESMF_DECLARE(typeObject, E_RandomNumberGenerator, "weibull", 2, 2,
				 std::weibull_distribution<double>(parameter[0].to<double>(runtime), parameter[1].to<double>(runtime))(**self))

}

}
}
