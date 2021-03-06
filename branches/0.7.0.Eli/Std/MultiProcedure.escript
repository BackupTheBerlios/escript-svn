// MultiProcedure.escript
// This file is part of the EScript programming language (http://escript.berlios.de)
//
// Copyright (C) 2013 Claudius Jähn <claudius@uni-paderborn.de>
//
// Licensed under the MIT License. See LICENSE file for details.
// ---------------------------------------------------------------------------------

loadOnce(__DIR__+"/basics.escript");

/*! Expendable function without result.
	\example
		var f = new Std.MultiProcedure;
		f+=fn(a){out( "x:",a,"\n" );};
		f+=fn(a){out( "y:",(a+1),"\n" );};
		f+=fn(a){out( "z:",(a+2),"\n"); return Std.MultiProcedure.REMOVE; }; // removed after one call

		f(27);
		// x:27
		// y:28
		// z:29
		f(27);
		// x:27
		// y:28
	*/

static T = new Type;

T._printableName @(override) ::= $MultiProcedure;
T.REMOVE ::= $REMOVE;
T.functions @(private,init) := Array;
T._call ::= fn(obj,params...){
	for(var i = 0;i<functions.count();){
		if( REMOVE == (obj->functions[i])(params...) ){
			functions.removeIndex(i);
		}else{
			++i;
		}
	}
};
T."+=" ::= 			fn(f){	this.functions += f;	};
T.accessFunctions ::=	fn(){	return functions;	};
T.clear ::=			fn(){	return functions.clear();	};
T.clone ::= fn(){
	var other = new (this.getType());
	(other->fn(f){	functions = f;	})(functions.clone());
	return other;
};
T.count ::=			fn(){	return functions.count();	};
T.empty ::=			fn(){	return functions.empty();	};
T.filter ::= fn(fun){
	functions.filter(fun);
	return this;
};

//!	\see Std.Traits.CallableTrait
Std.onModule('Std/Traits/CallableTrait', fn(CallableTrait){
	Std.require('Std/Traits/basics').addTrait( T, CallableTrait );
});

Std.onModule('Std/ObjectSerialization', fn(ObjectSerialization){
	ObjectSerialization.registerType(T,'Std.MultiProcedure')
		.enableIdentityTracking()
		.addDescriber(fn(ctxt,mFun,Map d){
			d['functions'] = ctxt.createDescription( mFun.accessFunctions() );
		})
		.addInitializer(fn(ctxt, mFun,Map d){
			var attr = d['functions'];
			if(attr){
				foreach(attr as var fnDescription)
					mFun += ctxt.createObject(fnDescription);
			}
		});
});

Std.MultiProcedure := T;
return T;
