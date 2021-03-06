// Compiler.h
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#ifndef ES_COMPILER_H
#define ES_COMPILER_H

#include "../Utils/CodeFragment.h"
#include "../Utils/Logger.h"
#include "../Utils/StringId.h"
#include "../Utils/StringData.h"
#include "../Instructions/Instruction.h"
#include "../Instructions/InstructionBlock.h"

namespace EScript {
	
class CompilerContext;
class Logger;
namespace AST{
class Statement;
}

/*! Compiler
	Input: Syntax tree made of Expressions
	Output: Byte Code made of EAssembler-Instructions */

class Compiler {
	public:
		Compiler(Logger * _logger = NULL);
	
		UserFunction * compile(const CodeFragment & code);
	
	// -------------
		
	//! @name Logging
	//	@{
	public:
		Logger * getLogger()const				{	return logger.get();	}
	private:
		_CountedRef<Logger> logger;
	//	@}


	
	// -------------
		
	//! @name Internal helper
	//	@{
	public:
		/*! (static,internal)
			- Replaces the markers inside the assembly by jump addresses.	*/
		static void finalizeInstructions( InstructionBlock & instructions ); 


		void compileExpression(CompilerContext & ctxt,ObjPtr expression)const;
		void compileStatement(CompilerContext & ctxt,const AST::Statement & statement)const;

		void throwError(CompilerContext & ctxt,const std::string & message)const;
	//	@}

};
}

#endif // ES_COMPILER_H
