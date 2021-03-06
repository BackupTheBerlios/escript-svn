// Compiler.cpp
// This file is part of the EScript programming language.
// See copyright notice in EScript.h
// ------------------------------------------------------
#include "Compiler.h"
#include "Parser.h"
#include "CompilerContext.h"
#include "../Consts.h"
#include "../Objects/typeIds.h"
#include "../Objects/AST/BlockExpr.h"
#include "../Objects/AST/ConditionalExpr.h"
#include "../Objects/AST/FunctionCallExpr.h"
#include "../Objects/AST/GetAttributeExpr.h"
#include "../Objects/AST/IfStatement.h"
#include "../Objects/AST/LogicOpExpr.h"
#include "../Objects/AST/LoopStatement.h"
#include "../Objects/AST/SetAttributeExpr.h"
#include "../Objects/AST/Statement.h"
#include "../Objects/AST/TryCatchStatement.h"
#include "../Objects/AST/UserFunctionExpr.h"
#include "../Objects/Callables/UserFunction.h"
#include "../Objects/Identifier.h"
#include "../Objects/Values/Bool.h"
#include "../Objects/Values/Number.h"
#include "../Objects/Values/String.h"
#include "../Objects/Values/Void.h"

#include <stdexcept>
#include <map>

#if !defined(_MSC_VER) and !defined(UNUSED_ATTRIBUTE)
#define UNUSED_ATTRIBUTE __attribute__ ((unused))
#else
#define UNUSED_ATTRIBUTE
#endif

namespace EScript{

// init handlerRegistry \todo Can't this be done more efficiently using c++11 functionals???
struct handler_t{ virtual void operator()(CompilerContext & ctxt,ObjPtr obj)=0; };
typedef std::map<internalTypeId_t,handler_t *> handlerRegistry_t;
static bool initHandler(handlerRegistry_t &);
static handlerRegistry_t handlerRegistry;
static bool _handlerInitialized UNUSED_ATTRIBUTE = initHandler(handlerRegistry);


//! (ctor)
Compiler::Compiler(Logger * _logger) : logger(_logger ? _logger : new StdLogger(std::cout)) {
}


void Compiler::compileExpression(CompilerContext & ctxt,ObjPtr expression)const{
	if(expression.isNull())
		return;
	const internalTypeId_t typeId = expression->_getInternalTypeId();

	handlerRegistry_t::iterator it = handlerRegistry.find(typeId);
	if(it==handlerRegistry.end()){
			std::cout << reinterpret_cast<void*>(typeId)<<"\n";
		throwError(ctxt,"Expression can't be compiled.");
	}
	(*it->second)(ctxt,expression);
}

void Compiler::throwError(CompilerContext & ctxt,const std::string & msg)const{
	std::ostringstream os;
	os << "Compiler: " << msg;
	Exception * e = new Exception(os.str(),ctxt.getCurrentLine());
	e->setFilename(ctxt.getCode().getFilename());
	throw e;
}


UserFunction * Compiler::compile(const CodeFragment & code){

	// prepare container function
	ERef<UserFunction> fun = new UserFunction;
	fun->setCode(code);

	// parse and build syntax tree
	Parser p(getLogger());
	ERef<AST::BlockExpr> block = p.parse(code);

	// outerBlock is used to add a return statement: {return {block}}
	ERef<AST::BlockExpr> outerBlock(new AST::BlockExpr);
	outerBlock->addStatement(AST::Statement(AST::Statement::TYPE_RETURN,block.get()));

	// compile and create instructions
	CompilerContext ctxt(*this,fun->getInstructionBlock(),code);
	ctxt.compile(outerBlock.get());
	Compiler::finalizeInstructions(fun->getInstructionBlock());

	return fun.detachAndDecrease();
}


//! (static)
void Compiler::finalizeInstructions( InstructionBlock & instructionBlock ){

	std::vector<Instruction> & instructions = instructionBlock._accessInstructions();

//	if(instructionBlock.hasJumpMarkers()){
		std::map<uint32_t,uint32_t> markerToPosition;

		{ // pass 1: remove setMarker-instructions and store position
			std::vector<Instruction> tmp;
			for(std::vector<Instruction>::const_iterator it=instructions.begin();it!=instructions.end();++it){
				if( it->getType() == Instruction::I_SET_MARKER ){
					markerToPosition[it->getValue_uint32()] = tmp.size();
				}else{
					tmp.push_back(*it);
				}
			}
			tmp.swap(instructions);
//			instructionBlock.clearMarkerNames();
		}

		{ // pass 2: adapt jump instructions
			for(std::vector<Instruction>::iterator it=instructions.begin();it!=instructions.end();++it){
				if( it->getType() == Instruction::I_JMP
						|| it->getType() == Instruction::I_JMP_IF_SET
						|| it->getType() == Instruction::I_JMP_ON_TRUE
						|| it->getType() == Instruction::I_JMP_ON_FALSE
						|| it->getType() == Instruction::I_SET_EXCEPTION_HANDLER){
					const uint32_t markerId = it->getValue_uint32();

					// is name of a marker (and not already a jump position)
					if(markerId>=Instruction::JMP_TO_MARKER_OFFSET){
						it->setValue_uint32(markerToPosition[markerId]);
					}
				}
			}

		}

//	}
}

//! (internal)
void Compiler::compileStatement(CompilerContext & ctxt,const AST::Statement & statement)const{
	using AST::Statement;
	if(statement.getLine()>=0)
		ctxt.setLine(statement.getLine());

	if(statement.getType() == Statement::TYPE_CONTINUE){
		const uint32_t target = ctxt.getCurrentMarker(CompilerContext::CONTINUE_MARKER);
		if(target==Instruction::INVALID_JUMP_ADDRESS){
			throwError(ctxt,"'continue' outside a loop.");
		}
		std::vector<size_t> variablesToReset;
		ctxt.collectLocalVariables(CompilerContext::CONTINUE_MARKER,variablesToReset);
		for(std::vector<size_t>::const_iterator it = variablesToReset.begin();it!=variablesToReset.end();++it){
			ctxt.addInstruction(Instruction::createResetLocalVariable(*it));
		}
		ctxt.addInstruction(Instruction::createJmp(target));

	}else if(statement.getType() == Statement::TYPE_BREAK){
		const uint32_t target = ctxt.getCurrentMarker(CompilerContext::BREAK_MARKER);
		if(target==Instruction::INVALID_JUMP_ADDRESS){
			throwError(ctxt,"'break' outside a loop.");
		}
		std::vector<size_t> variablesToReset;
		ctxt.collectLocalVariables(CompilerContext::BREAK_MARKER,variablesToReset);
		for(std::vector<size_t>::const_iterator it = variablesToReset.begin();it!=variablesToReset.end();++it){
			ctxt.addInstruction(Instruction::createResetLocalVariable(*it));
		}
		ctxt.addInstruction(Instruction::createJmp(target));
	}else if(statement.getType() == Statement::TYPE_EXIT){
		if(statement.getExpression().isNotNull()){
			ctxt.compile(statement.getExpression());
		}
		ctxt.addInstruction(Instruction::createPushUInt(Consts::SYS_CALL_EXIT));
		ctxt.addInstruction(Instruction::createSysCall(statement.getExpression().isNotNull() ? 1 : 0));

	}else if(statement.getType() == Statement::TYPE_EXPRESSION){
//		ctxt.setLine(statement.getLine());
		ctxt.compile(statement.getExpression());
		ctxt.addInstruction(Instruction::createPop());

	}else if(statement.getType() == Statement::TYPE_RETURN){
		if(statement.getExpression().isNotNull()){
			ctxt.compile(statement.getExpression());
			ctxt.addInstruction(Instruction::createAssignLocal(Consts::LOCAL_VAR_INDEX_internalResult));
		}
		ctxt.addInstruction(Instruction::createJmp(Instruction::INVALID_JUMP_ADDRESS));

	}else if(statement.getType() == Statement::TYPE_STATEMENT){
//		ctxt.setLine(statement.getLine());

		// block - statement (NOT block - expression)
		if(statement.getExpression().isNotNull() && statement.getExpression()->_getInternalTypeId() == _TypeIds::TYPE_BLOCK_STATEMENT ){
			const AST::BlockExpr * blockStatement = statement.getExpression().toType<AST::BlockExpr>();

			if(blockStatement->hasLocalVars())
				ctxt.pushSetting_localVars(blockStatement->getVars());

			for ( AST::BlockExpr::cStatementCursor c = blockStatement->getStatements().begin();  c != blockStatement->getStatements().end(); ++c) {
					ctxt.compile(*c);
			}
			if(blockStatement->hasLocalVars()){
				for(std::set<StringId>::const_iterator it = blockStatement->getVars().begin();it!=blockStatement->getVars().end();++it){
					ctxt.addInstruction(Instruction::createResetLocalVariable(ctxt.getCurrentVarIndex(*it)));
				}
				ctxt.popSetting();
			}
		}else{
			ctxt.compile(statement.getExpression());
		}

	}else if(statement.getType() == Statement::TYPE_THROW){
		if(statement.getExpression().isNotNull()){
			ctxt.compile(statement.getExpression());
		}
		ctxt.addInstruction(Instruction::createPushUInt(Consts::SYS_CALL_THROW));
		ctxt.addInstruction(Instruction::createSysCall(statement.getExpression().isNotNull() ? 1 : 0));

	}else if(statement.getType() == Statement::TYPE_YIELD){
		if(statement.getExpression().isNotNull()){
			ctxt.compile(statement.getExpression());
		}else{
			ctxt.addInstruction(Instruction::createPushVoid());
		}
		ctxt.addInstruction(Instruction::createYield());

	}else if(statement.getExpression().isNotNull()){
		throwError(ctxt,"Unknown statement.");
	}
}



// ------------------------------------------------------------------


//! (static)
bool initHandler(handlerRegistry_t & m){
	// \note  the redundant assignment to 'id2' is a workaround to a strange linker error ("undefined reference EScript::_TypeIds::TYPE_NUMBER")
	#define ADD_HANDLER( _id, _type, _block) \
	{ \
		struct _handler : public handler_t{ \
			~_handler(){} \
			virtual void operator()(CompilerContext & ctxt,ObjPtr obj){ \
				_type * self = obj.toType<_type>(); \
				if(!self) throw std::invalid_argument("Wrong type!"); \
				do _block while(false); \
			} \
		}; \
		const internalTypeId_t id2 = _id; \
		m[id2] = new _handler(); \
	}
	// ------------------------
	// Simple types

	// Bool
	ADD_HANDLER( _TypeIds::TYPE_BOOL, Bool, {
		ctxt.addInstruction(Instruction::createPushBool(self->toBool()));
	})
	// Identifier
	ADD_HANDLER( _TypeIds::TYPE_IDENTIFIER, Identifier, {
		ctxt.addInstruction(Instruction::createPushId(self->getId()));
	})
	// Number
	ADD_HANDLER( _TypeIds::TYPE_NUMBER, Number, {
		ctxt.addInstruction(Instruction::createPushNumber(self->toDouble()));
	})

	// String
	ADD_HANDLER( _TypeIds::TYPE_STRING, String, {
		ctxt.addInstruction(Instruction::createPushString(ctxt.declareString(self->toString())));
	})
	// Void
	ADD_HANDLER( _TypeIds::TYPE_VOID, Void, {
		ctxt.addInstruction(Instruction::createPushVoid());
	})


	// ------------------------
	// AST
	using namespace AST;

	ADD_HANDLER( _TypeIds::TYPE_BLOCK_STATEMENT, BlockExpr, {
		if(self->hasLocalVars())
			ctxt.pushSetting_localVars(self->getVars());

		if(self->getStatements().empty()){
			ctxt.addInstruction(Instruction::createPushVoid());
		}else{
			for ( BlockExpr::cStatementCursor c = self->getStatements().begin();  c != self->getStatements().end(); ++c) {
				if(c+1 == self->getStatements().end()){ // last statemenet ? --> keep the result
					if( c->getType() == Statement::TYPE_EXPRESSION ){
						ctxt.compile( c->getExpression() );
					}else{
						ctxt.compile(*c);
						ctxt.addInstruction(Instruction::createPushVoid());
					}
				}else{
					ctxt.compile(*c);
				}
			}
		}
		if(self->hasLocalVars()){
			for(std::set<StringId>::const_iterator it = self->getVars().begin();it!=self->getVars().end();++it){
				ctxt.addInstruction(Instruction::createResetLocalVariable(ctxt.getCurrentVarIndex(*it)));
			}
			ctxt.popSetting();
		}
	})

	// ConditionalExpr
	ADD_HANDLER( _TypeIds::TYPE_CONDITIONAL_EXPRESSION, ConditionalExpr, {
		if(self->getCondition().isNull()){
			if(self->getElseAction().isNotNull()){
				ctxt.compile(self->getElseAction());
			}
		}else{
			const uint32_t elseMarker = ctxt.createMarker();

			ctxt.compile(self->getCondition());
			ctxt.addInstruction(Instruction::createJmpOnFalse(elseMarker));

			ctxt.compile( self->getAction() );

			if(self->getElseAction().isNotNull()){
				const uint32_t endMarker = ctxt.createMarker();
				ctxt.addInstruction(Instruction::createJmp(endMarker));
				ctxt.addInstruction(Instruction::createSetMarker(elseMarker));
				ctxt.compile( self->getElseAction() );
				ctxt.addInstruction(Instruction::createSetMarker(endMarker));
			}else{
				ctxt.addInstruction(Instruction::createSetMarker(elseMarker));
			}
		}
	})

	// FunctionCallExpr
	ADD_HANDLER( _TypeIds::TYPE_FUNCTION_CALL_EXPRESSION, FunctionCallExpr, {
		ctxt.setLine(self->getLine());

		if(!self->isSysCall()){
			do{
				GetAttributeExpr * gAttr = self->getGetFunctionExpression().toType<GetAttributeExpr>();

				// getAttributeExpression (...)
				if( gAttr ){
					const StringId attrId = gAttr->getAttrId();

					if(gAttr->getObjectExpression()==NULL){ // singleIdentifier (...)
						const int localVarIndex = ctxt.getCurrentVarIndex(attrId);
						if(localVarIndex>=0){
							if( !self->isConstructorCall() ){ // constructor calls don't need a caller
								ctxt.addInstruction(Instruction::createPushVoid());
							}
							ctxt.addInstruction(Instruction::createGetLocalVariable(localVarIndex));
						}else{
							if( self->isConstructorCall() ){ // constructor calls don't need a caller
								ctxt.addInstruction(Instruction::createGetVariable(attrId));
							}else{
								ctxt.addInstruction(Instruction::createFindVariable(attrId));
							}
						}
						break;
					} // getAttributeExpression.identifier (...)
					else if(GetAttributeExpr * gAttrGAttr = gAttr->getObjectExpression().toType<GetAttributeExpr>() ){
						ctxt.compile(gAttrGAttr);
						if( !self->isConstructorCall() ){ // constructor calls don't need a caller
							ctxt.addInstruction(Instruction::createDup());
						}
						ctxt.addInstruction(Instruction::createGetAttribute(attrId));
						break;
					} // somethingElse.identifier (...) e.g. foo().bla(), 7.bla()
					else{
						ctxt.compile(gAttr->getObjectExpression());
						if( !self->isConstructorCall() ){ // constructor calls don't need a caller
							ctxt.addInstruction(Instruction::createDup());
						}
						ctxt.addInstruction(Instruction::createGetAttribute(attrId));
						break;
					}
				}else{
					if( !self->isConstructorCall() ){ // constructor calls don't need a caller
						ctxt.addInstruction(Instruction::createPushVoid());
					}
					ctxt.compile(self->getGetFunctionExpression());
					break;
				}

			}while(false);
		}
		for(std::vector<ObjRef>::const_iterator it=self->getParams().begin();it!=self->getParams().end();++it){
			if( it->isNull() ){
				// push undefined to be able to distinguish 'someFun(void,2);' from 'someFun(,2);'
				ctxt.addInstruction(Instruction::createPushUndefined());
			}else{
				ctxt.compile(*it);
			}
		}
		if( self->isSysCall()){
			ctxt.addInstruction(Instruction::createPushUInt(self->getSysCallId()));
			ctxt.addInstruction(Instruction::createSysCall(self->getParams().size()));
		}else if( self->isConstructorCall()){
			ctxt.addInstruction(Instruction::createCreateInstance(self->getParams().size()));
		}else{
			ctxt.addInstruction(Instruction::createCall(self->getParams().size()));
		}
	})

	// GetAttributeExpr
	ADD_HANDLER( _TypeIds::TYPE_GET_ATTRIBUTE_EXPRESSION, GetAttributeExpr, {
		if(self->getObjectExpression().isNotNull()){
			ctxt.compile(self->getObjectExpression());
			ctxt.addInstruction(Instruction::createGetAttribute(self->getAttrId()));
		}else{
			const int localVarIndex = ctxt.getCurrentVarIndex(self->getAttrId());
			if(localVarIndex>=0){
				ctxt.addInstruction(Instruction::createGetLocalVariable(localVarIndex));
			}else{
				ctxt.addInstruction(Instruction::createGetVariable(self->getAttrId()));
			}
		}

	})

	// IfStatement
	ADD_HANDLER( _TypeIds::TYPE_IF_STATEMENT, IfStatement, {
		if(self->getCondition().isNull()){
			if(self->getElseAction().isValid()){
				ctxt.compile(self->getElseAction());
			}
		}else{
			const uint32_t elseMarker = ctxt.createMarker();

			ctxt.compile(self->getCondition());
			ctxt.addInstruction(Instruction::createJmpOnFalse(elseMarker));
			if(self->getAction().isValid()){
				ctxt.compile(self->getAction());
			}

			if(self->getElseAction().isValid()){
				const uint32_t endMarker = ctxt.createMarker();
				ctxt.addInstruction(Instruction::createJmp(endMarker));
				ctxt.addInstruction(Instruction::createSetMarker(elseMarker));
				ctxt.compile(self->getElseAction());
				ctxt.addInstruction(Instruction::createSetMarker(endMarker));
			}else{
				ctxt.addInstruction(Instruction::createSetMarker(elseMarker));
			}
		}
	})


	// IfStatement
	ADD_HANDLER( _TypeIds::TYPE_LOGIC_OP_EXPRESSION, LogicOpExpr, {
		switch(self->getOperator()){
			case LogicOpExpr::NOT:{
				ctxt.compile(self->getLeft());
				ctxt.addInstruction(Instruction::createNot());
				break;
			}
			case LogicOpExpr::OR:{
				const uint32_t marker = ctxt.createMarker();
				const uint32_t endMarker = ctxt.createMarker();
				ctxt.compile(self->getLeft());
				ctxt.addInstruction(Instruction::createJmpOnTrue(marker));
				ctxt.compile(self->getRight());
				ctxt.addInstruction(Instruction::createJmpOnTrue(marker));
				ctxt.addInstruction(Instruction::createPushBool(false));
				ctxt.addInstruction(Instruction::createJmp(endMarker));
				ctxt.addInstruction(Instruction::createSetMarker(marker));
				ctxt.addInstruction(Instruction::createPushBool(true));
				ctxt.addInstruction(Instruction::createSetMarker(endMarker));
				break;
			}
			default:
			case LogicOpExpr::AND:{
				const uint32_t marker = ctxt.createMarker();
				const uint32_t endMarker = ctxt.createMarker();
				ctxt.compile(self->getLeft());
				ctxt.addInstruction(Instruction::createJmpOnFalse(marker));
				ctxt.compile(self->getRight());
				ctxt.addInstruction(Instruction::createJmpOnFalse(marker));
				ctxt.addInstruction(Instruction::createPushBool(true));
				ctxt.addInstruction(Instruction::createJmp(endMarker));
				ctxt.addInstruction(Instruction::createSetMarker(marker));
				ctxt.addInstruction(Instruction::createPushBool(false));
				ctxt.addInstruction(Instruction::createSetMarker(endMarker));
				break;
			}
		}
	})

	// for-statement
	ADD_HANDLER( _TypeIds::TYPE_LOOP_STATEMENT, LoopStatement, {
		const uint32_t loopBegin = ctxt.createMarker();
		const uint32_t loopEndMarker = ctxt.createMarker();
		const uint32_t loopContinueMarker = ctxt.createMarker();

		if(self->getInitStatement().isValid()){
			ctxt.setLine(self->getInitStatement().getLine());
			ctxt.compile(self->getInitStatement());
		}
		ctxt.addInstruction(Instruction::createSetMarker(loopBegin));

		if(self->getPreConditionExpression().isNotNull()){
			ctxt.compile(self->getPreConditionExpression());
			ctxt.addInstruction(Instruction::createJmpOnFalse(loopEndMarker));
		}
		ctxt.pushSetting_marker( CompilerContext::BREAK_MARKER ,loopEndMarker);
		ctxt.pushSetting_marker( CompilerContext::CONTINUE_MARKER ,loopContinueMarker);
		ctxt.compile(self->getAction());
		ctxt.popSetting();
		ctxt.popSetting();

		if(self->getPostConditionExpression().isNotNull()){ // increaseStmt is ignored!
			ctxt.addInstruction(Instruction::createSetMarker(loopContinueMarker));
			ctxt.compile(self->getPostConditionExpression());
			ctxt.addInstruction(Instruction::createJmpOnTrue(loopBegin));
		}else{
			ctxt.addInstruction(Instruction::createSetMarker(loopContinueMarker));
			if(self->getIncreaseStatement().isValid()){
				ctxt.compile(self->getIncreaseStatement());
			}
			ctxt.addInstruction(Instruction::createJmp(loopBegin));
		}
		ctxt.addInstruction(Instruction::createSetMarker(loopEndMarker));
	})


	// if-statement
	ADD_HANDLER( _TypeIds::TYPE_SET_ATTRIBUTE_EXPRESSION, SetAttributeExpr, {
		ctxt.compile(self->getValueExpression());

		ctxt.setLine(self->getLine());
		ctxt.addInstruction(Instruction::createDup());

		const StringId attrId = self->getAttrId();
		if(self->isAssignment()){
			// no object given: a = ...
			if(self->getObjectExpression().isNull()){
				// local variable: var a = ...
				if(ctxt.getCurrentVarIndex(attrId)>=0){
					ctxt.addInstruction(Instruction::createAssignLocal(ctxt.getCurrentVarIndex(attrId)));
				}else{
					ctxt.addInstruction(Instruction::createAssignVariable(attrId));
				}
			}else{ // object.a =
				ctxt.compile(self->getObjectExpression());
				ctxt.addInstruction(Instruction::createAssignAttribute(attrId));
			}

		}else{
				ctxt.compile(self->getObjectExpression());
				ctxt.addInstruction(Instruction::createPushUInt(static_cast<uint32_t>(self->getAttributeProperties())));
				ctxt.addInstruction(Instruction::createSetAttribute(attrId));
		}
	})

	// TryCatchStatement
	ADD_HANDLER( _TypeIds::TYPE_TRY_CATCH_STATEMENT, TryCatchStatement, {
		const uint32_t catchMarker = ctxt.createMarker();
		const uint32_t endMarker = ctxt.createMarker();

		// try
		// ------
		ctxt.pushSetting_marker(CompilerContext::EXCEPTION_MARKER,catchMarker);
		ctxt.addInstruction(Instruction::createSetExceptionHandler(catchMarker));

		// collect all variables that are declared inside the try-block (excluding nested try-blocks)
		std::vector<size_t> collectedVariableIndices;
		ctxt.pushLocalVarsCollector(&collectedVariableIndices);
		ctxt.compile(self->getTryBlock());
		ctxt.popLocalVarsCollector();

		ctxt.popSetting(); // restore previous EXCEPTION_MARKER

		// try block without exception --> reset catchMarker and jump to endMarker
		ctxt.addInstruction(Instruction::createSetExceptionHandler(ctxt.getCurrentMarker(CompilerContext::EXCEPTION_MARKER)));
		ctxt.addInstruction(Instruction::createJmp(endMarker));

		// catch
		// ------
		const StringId exceptionVariableName = self->getExceptionVariableName();

		ctxt.addInstruction(Instruction::createSetMarker(catchMarker));
		// reset catchMarker
		ctxt.addInstruction(Instruction::createSetExceptionHandler(ctxt.getCurrentMarker(CompilerContext::EXCEPTION_MARKER)));

		// clear all variables defined inside try block
		for(std::vector<size_t>::const_iterator it = collectedVariableIndices.begin(); it!=collectedVariableIndices.end();++it){
			ctxt.addInstruction(Instruction::createResetLocalVariable(*it));
		}

		// define exception variable
		if(!exceptionVariableName.empty()){
			std::set<StringId> varSet;
			varSet.insert(exceptionVariableName);
			ctxt.pushSetting_localVars(varSet);
			// load exception-variable with exception object ( exceptionVariableName = __result )
			ctxt.addInstruction(Instruction::createGetLocalVariable(Consts::LOCAL_VAR_INDEX_internalResult));
			ctxt.addInstruction(Instruction::createAssignLocal(ctxt.getCurrentVarIndex(exceptionVariableName)));
		}

		// clear the exception-variable
		ctxt.addInstruction(Instruction::createResetLocalVariable(Consts::LOCAL_VAR_INDEX_internalResult));

		// execute catch block
		ctxt.compile(self->getCatchBlock());
		// pop exception variable
		if(!exceptionVariableName.empty()){
			ctxt.addInstruction(Instruction::createResetLocalVariable(ctxt.getCurrentVarIndex(exceptionVariableName)));
			ctxt.popSetting(); // variable
		}
		// end:
		ctxt.addInstruction(Instruction::createSetMarker(endMarker));
	})

	// user function
	ADD_HANDLER( _TypeIds::TYPE_USER_FUNCTION_EXPRESSION, UserFunctionExpr, {

		ERef<UserFunction> fun = new UserFunction;
		fun->setParameterCounts(self->getParamList().size(),self->getMinParamCount(),self->getMaxParamCount());
		fun->setCode(self->getCode());
		fun->setLine(self->getLine());

		CompilerContext ctxt2(ctxt.getCompiler(),fun->getInstructionBlock(),self->getCode());
		ctxt2.setLine(self->getLine()); // set the line of all initializations to the line of the function declaration

		// declare a local variables for each parameter expression
		for(UserFunctionExpr::parameterList_t::const_iterator it = self->getParamList().begin();it!=self->getParamList().end();++it){
			fun->getInstructionBlock().declareLocalVariable( it->getName() );
		}

		ctxt2.pushSetting_basicLocalVars(); // make 'this' and parameters available

		// default parameters
		for(UserFunctionExpr::parameterList_t::const_iterator it = self->getParamList().begin();it!=self->getParamList().end();++it){
			const UserFunctionExpr::Parameter & param = *it;
			ObjPtr defaultExpr = param.getDefaultValueExpression();
			if(defaultExpr.isNotNull()){
				const int varIdx = ctxt2.getCurrentVarIndex(param.getName()); // \todo assert(varIdx>=0)

				const uint32_t parameterAvailableMarker = ctxt2.createMarker();
				ctxt2.addInstruction(Instruction::createPushUInt(varIdx));
				ctxt2.addInstruction(Instruction::createJmpIfSet(parameterAvailableMarker));

//					ctxt2.enableGlobalVarContext();				// \todo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				ctxt2.compile(defaultExpr);
//					ctxt2.disableGlobalVarContext();			// \todo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				ctxt2.addInstruction(Instruction::createAssignLocal(varIdx));

				ctxt2.addInstruction(Instruction::createSetMarker(parameterAvailableMarker));
			}
		}

		// parameter type checks
		for(UserFunctionExpr::parameterList_t::const_iterator it = self->getParamList().begin();it!=self->getParamList().end();++it){
			const UserFunctionExpr::Parameter & param = *it;
			const std::vector<ObjRef> & typeExpressions = param.getTypeExpressions();
			if(typeExpressions.empty())
				continue;
			const int varIdx = ctxt2.getCurrentVarIndex(param.getName());	// \todo assert(varIdx>=0)



			// if the parameter has value constrains AND is a multi parameter, use a special system-call for this (instead of manually creating a foreach-loop here)
			// e.g. fn([Bool,Number] p*){...}
			if(param.isMultiParam()){
				for(std::vector<ObjRef>::const_iterator it2 = typeExpressions.begin();it2!=typeExpressions.end();++it2){
					ctxt2.compile( *it2 );
				}
				ctxt2.addInstruction(Instruction::createGetLocalVariable(varIdx));
				ctxt2.addInstruction(Instruction::createPushUInt(Consts::SYS_CALL_TEST_ARRAY_PARAMETER_CONSTRAINTS));
				ctxt2.addInstruction(Instruction::createSysCall( typeExpressions.size()+1 ));
				ctxt2.addInstruction(Instruction::createPop());

			}else{
				std::vector<uint32_t> constrainOkMarkers; // each constrain gets its own ok-marker
				for(std::vector<ObjRef>::const_iterator it2 = typeExpressions.begin();it2!=typeExpressions.end();++it2){
					const uint32_t constrainOkMarker = ctxt2.createMarker();
					constrainOkMarkers.push_back(constrainOkMarker);

					ctxt2.compile( *it2 );
					ctxt2.addInstruction(Instruction::createDup()); // store the constraint for the error message
					ctxt2.addInstruction(Instruction::createCheckType(varIdx));
					ctxt2.addInstruction(Instruction::createJmpOnTrue(constrainOkMarker));
				}

				// all constraint-checks failed! -> stack contains all failed constraints
				ctxt2.addInstruction(Instruction::createGetLocalVariable(varIdx));
				ctxt2.addInstruction(Instruction::createPushUInt(Consts::SYS_CALL_THROW_TYPE_EXCEPTION));
				ctxt2.addInstruction(Instruction::createSysCall( constrainOkMarkers.size()+1 ));
				ctxt2.addInstruction(Instruction::createJmp( Instruction::INVALID_JUMP_ADDRESS ));

				// depending on which constraint-check succeeded, pop the constraint-values from the stack
				for(std::vector<uint32_t>::const_reverse_iterator cIt = constrainOkMarkers.rbegin();cIt!=constrainOkMarkers.rend();++cIt){
					ctxt2.addInstruction(Instruction::createSetMarker(*cIt));
					ctxt2.addInstruction(Instruction::createPop());
				}
			}
		}

		// add super-constructor parameters
		const std::vector<ObjRef> & superConstrParams = self->getSConstructorExpressions();
		for(std::vector<ObjRef>::const_iterator it = superConstrParams.begin();it!=superConstrParams.end();++it)
			ctxt2.compile(*it);

		// init 'this' (or create it if this is a constructor call)
		ctxt2.addInstruction(Instruction::createInitCaller(superConstrParams.size()));

		ctxt2.compile(Statement(Statement::TYPE_STATEMENT,self->getBlock()));
		ctxt2.popSetting();
		Compiler::finalizeInstructions(fun->getInstructionBlock());

		ctxt.addInstruction(Instruction::createPushFunction(ctxt.registerInternalFunction(fun.get())));

	})

	// ------------------------
	#undef ADD_HANDLER
	return true;
}

}
