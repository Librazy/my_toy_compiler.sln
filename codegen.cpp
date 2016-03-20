#include "node.h"
#include "codegen.h"
#include "parser.hpp"

using namespace std;
static Function* fun = nullptr;

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	std::cout << "Generating code...\n";

	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	FunctionType* ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), makeArrayRef(argTypes), false);
	mainFunction = Function::Create(ftype, GlobalValue::InternalLinkage, "main", module);
	BasicBlock* bblock = BasicBlock::Create(getGlobalContext(), "entry", mainFunction, nullptr);

	/* Push a new variable/block context */
	pushBlock(bblock);
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	ReturnInst::Create(getGlobalContext(), bblock);
	popBlock();

	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::cout << "Code is generated.\n";
	PassManager<Module> pm;
	pm.addPass(PrintModulePass(outs()));
	pm.run(*module);
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() const
{
	std::cout << "Running code...\n";
	ExecutionEngine* ee = EngineBuilder(unique_ptr<Module>(module)).create();
	ee->finalizeObject();
	vector<GenericValue> noargs;
	GenericValue v = ee->runFunction(mainFunction, noargs);
	std::cout << "Code was run.\n";
	return v;
}

/* Returns an LLVM type based on the identifier */
static Type* typeOf(const NIdentifier& type)
{
	if (type.name.compare("int") == 0) {
		return Type::getInt64Ty(getGlobalContext());
	}
	else if (type.name.compare("double") == 0) {
		return Type::getDoubleTy(getGlobalContext());
	}
	return Type::getVoidTy(getGlobalContext());
}

static inline Value* cast(Value* op, Type* dest, CodeGenContext& context)
{
	if (op->getType() != Type::getDoubleTy(getGlobalContext())) {
		assert(dest == Type::getDoubleTy(getGlobalContext()) && "Must be an cast to Double!");
		return new SIToFPInst(op, dest, "", context.currentBlock());
	}
	else {
		assert(op->getType() == Type::getDoubleTy(getGlobalContext()) && "Must be either a Int64 or Double");
		assert(dest == Type::getInt64Ty(getGlobalContext()) && "Must be an cast to Int64!");
		return new FPToSIInst(op, dest, "", context.currentBlock());
	}
}

static inline Value* castToIfNeed(Value* op, Type* dest, CodeGenContext& context)
{
	Value* res;
	if (op->getType() != dest) {
		res = cast(op, dest, context);
	}
	else res = op;
	return res;
}
static inline Value* ctinDouble(Value* op,CodeGenContext& context)
{
	return castToIfNeed(op, Type::getDoubleTy(getGlobalContext()),context);
}

static inline Value* ctinInt64(Value* op, CodeGenContext& context)
{
	return castToIfNeed(op, Type::getInt64Ty(getGlobalContext()), context);
}
/* -- Code Generation -- */

Value* NInteger::codeGen(CodeGenContext& context)
{
	std::cout << "Creating integer: " << value << endl;
	return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context)
{
	std::cout << "Creating double: " << value << endl;
	return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context)
{
	std::cout << "Creating identifier reference: " << name << endl;
	if (context.locals().find(name) == context.locals().end()) {
		std::cerr << "undeclared variable " << name << endl;
		return nullptr;
	}
	return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context)
{
	Function* function = context.module->getFunction(id.name.c_str());
	if (function == nullptr) {
		std::cerr << "no such function " << id.name << endl;
	}
	std::vector<Value*> args;
	ExpressionList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); ++it) {
		args.push_back((**it).codeGen(context));
	}
	CallInst* call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
	std::cout << "Creating method call: " << id.name << endl;
	return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
	std::cout << "Creating binary operation " << op << endl;
	Instruction::BinaryOps instr;
	Value* lhs_v = lhs.codeGen(context);
	Value* rhs_v = rhs.codeGen(context);
	if (lhs_v->getType() == Type::getInt64Ty(getGlobalContext()) && rhs_v->getType() == Type::getInt64Ty(getGlobalContext())) {
		switch (op) {
		case TPLUS: instr = Instruction::Add;
			goto math;
		case TMINUS: instr = Instruction::Sub;
			goto math;
		case TMUL: instr = Instruction::Mul;
			goto math;
		case TDIV: instr = Instruction::SDiv;
			goto math;
		case TPOW:
			std::vector<Type *> arg_type;
			std::vector<Value *> arg;

			Value* lhs_v_d = ctinDouble(lhs_v, context);
			Value* rhs_v_d = ctinDouble(rhs_v, context);
			arg.push_back(lhs_v_d);
			arg.push_back(rhs_v_d);

			arg_type.push_back(Type::getDoubleTy(getGlobalContext()));
			arg_type.push_back(Type::getDoubleTy(getGlobalContext()));

			if (!fun){fun = getDeclaration(context.module, Intrinsic::pow, makeArrayRef(arg_type)); fun->setName("llvm.pow.f64");}
			Value* res_D = CallInst::Create(fun, makeArrayRef(arg), "llvm.pow.f64", context.currentBlock());
			return ctinInt64(res_D,context);
			/* TODO comparison */
		}

		return nullptr;
	math:
		return BinaryOperator::Create(instr, lhs_v,
			rhs_v, "", context.currentBlock());
	}else {
		Value* lhs_v_d = ctinDouble(lhs_v, context);
		Value* rhs_v_d = ctinDouble(rhs_v, context);
		switch (op) {
		case TPLUS: instr = Instruction::FAdd;
			goto mathd;
		case TMINUS: instr = Instruction::FSub;
			goto mathd;
		case TMUL: instr = Instruction::FMul;
			goto mathd;
		case TDIV: instr = Instruction::FDiv;
			goto mathd;
		case TPOW:
			std::vector<Type *> arg_type;
			std::vector<Value *> arg;

			arg.push_back(lhs_v_d);
			arg.push_back(rhs_v_d);

			arg_type.push_back(Type::getDoubleTy(getGlobalContext()));
			arg_type.push_back(Type::getDoubleTy(getGlobalContext()));

			if (!fun){fun = getDeclaration(context.module, Intrinsic::pow, makeArrayRef(arg_type)); fun->setName("llvm.pow.f64");}
			return CallInst::Create(fun, makeArrayRef(arg), "llvm.pow.f64", context.currentBlock());
			/* TODO comparison */
		}
	mathd:
		return BinaryOperator::Create(instr, lhs_v_d,
			rhs_v_d, "", context.currentBlock());
	}

}

Value* NAssignment::codeGen(CodeGenContext& context)
{
	std::cout << "Creating assignment for " << lhs.name << endl;
	if (context.locals().find(lhs.name) == context.locals().end()) {
		std::cerr << "undeclared variable " << lhs.name << endl;
		return nullptr;
	}
	return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context)
{
	StatementList::const_iterator it;
	Value* last = nullptr;
	for (it = statements.begin(); it != statements.end(); ++it) {
		std::cout << "Generating code for " << typeid(**it).name() << endl;
		last = (**it).codeGen(context);
	}
	std::cout << "Creating block" << endl;
	return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating code for " << typeid(expression).name() << endl;
	return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating return code for " << typeid(expression).name() << endl;
	Value* returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NVariableDeclaration::codeGen(CodeGenContext& context)
{
	std::cout << "Creating variable declaration " << type.name << " " << id.name << endl;
	AllocaInst* alloc = new AllocaInst(typeOf(type), id.name.c_str(), context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != nullptr) {
		NAssignment assn(id, *assignmentExpr);
		assn.codeGen(context);
	}
	return alloc;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context)
{
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); ++it) {
		argTypes.push_back(typeOf((**it).type));
	}
	FunctionType* ftype = FunctionType::get(typeOf(type), makeArrayRef(argTypes), false);
	Function* function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.module);
	return function;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); ++it) {
		argTypes.push_back(typeOf((**it).type));
	}
	FunctionType* ftype = FunctionType::get(typeOf(type), makeArrayRef(argTypes), false);
	Function* function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
	BasicBlock* bblock = BasicBlock::Create(getGlobalContext(), "entry", function, nullptr);

	context.pushBlock(bblock);

	Function::arg_iterator argsValues = function->arg_begin();
	Value* argumentValue;
	for (it = arguments.begin(); it != arguments.end(); ++it) {
		(**it).codeGen(context);

		argumentValue = argsValues.getNodePtrUnchecked();
		argumentValue->setName((*it)->id.name.c_str());
		StoreInst* inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
	}

	block.codeGen(context);
	ReturnInst::Create(getGlobalContext(), context.getCurrentReturnValue(), bblock);

	context.popBlock();
	std::cout << "Creating function: " << id.name << endl;
	return function;
}

