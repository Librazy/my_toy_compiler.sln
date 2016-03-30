#include "node.h"
#include "codegen.h"
#include "parser.hpp"

using namespace std;

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
	pushBlock(bblock,"main");
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	ReturnInst::Create(getGlobalContext(), currentBlock());
	popBlock();

	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::cout << "Code is generated.\n";
	PassManager<Module> pm;
	AnalysisManager<Module>* am = new AnalysisManager<Module>;
	pm.addPass(PrintModulePass(outs()));
	pm.run(*module,*am);
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
	else if (type.name.compare("bool") == 0) {
		return Type::getInt1Ty(getGlobalContext());
	}
	return Type::getVoidTy(getGlobalContext());
}

static inline Value* cast(Value* op, Type* dest, CodeGenContext& context)
{
	if (op->getType() == Type::getInt64Ty(getGlobalContext()) &&dest == Type::getDoubleTy(getGlobalContext())) {
		//Int64 -> Double
		return new SIToFPInst(op, dest, "", context.currentBlock());
	} else if (op->getType() == Type::getDoubleTy(getGlobalContext()) && dest == Type::getInt64Ty(getGlobalContext())) {
		//Double -> Int64
		return new FPToSIInst(op, dest, "", context.currentBlock());
	} else if (op->getType() == Type::getInt64Ty(getGlobalContext()) && dest == Type::getInt1Ty(getGlobalContext())) {
		//Int64 -> Boolean
		return ICmpInst::Create(Instruction::ICmp, ICmpInst::Predicate::ICMP_NE, op, ConstantInt::get(Type::getInt64Ty(getGlobalContext()),0),
			"", context.currentBlock());
	} else if (op->getType() == Type::getDoubleTy(getGlobalContext()) && dest == Type::getInt1Ty(getGlobalContext())) {
		//Double -> Boolean
		return FCmpInst::Create(Instruction::FCmp, ICmpInst::Predicate::FCMP_ONE, op, ConstantFP::get(Type::getDoubleTy(getGlobalContext()), 0.0),
			"", context.currentBlock());
	}
	llvm_unreachable("Invaild cast!");
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

static inline Value* ctinBoolean(Value* op, CodeGenContext& context)
{
	return castToIfNeed(op, Type::getInt1Ty(getGlobalContext()), context);
}
/* -- Code Generation -- */

Value* NBool::codeGen(CodeGenContext& context)
{
	std::cout << "Creating boolean: " << value << endl;
	return ConstantInt::get(Type::getInt1Ty(getGlobalContext()), value, true);
}

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
	Value* loc;
	if (!((loc = context.find_locals(name)))) {
		std::cerr << "undeclared variable " << name << endl;
		return nullptr;
	}
	return new LoadInst(loc, "", false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context)
{
	Function* function = context.module->getFunction(id.name.c_str());
	if (function == nullptr) {
		std::cerr << "no such function " << id.name << endl;
	}
	std::cout << "Creating method call: " << id.name << endl;
	std::vector<Value*> args;
	ExpressionList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); ++it) {

		args.push_back((**it).codeGen(context));
		std::cout << "    arg of type " << args.back()->getType() << endl;
	}
	CallInst* call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());

	return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
	std::cout << "Creating binary operation " << op << endl;
	Instruction::BinaryOps instr;
	Instruction::OtherOps cmpinstr;
	ICmpInst::Predicate pred;
	Value* lhs_v = lhs.codeGen(context);
	Value* rhs_v = rhs.codeGen(context);
	std::cout << "The operands' types are " << lhs_v->getType()<< " and " << rhs_v->getType() << endl;
	if (lhs_v->getType() == Type::getInt64Ty(getGlobalContext()) && rhs_v->getType() == Type::getInt64Ty(getGlobalContext())) {
		cmpinstr = Instruction::ICmp;
		switch (op) {
		case TPLUS: instr = Instruction::Add;
			goto math;
		case TMINUS: instr = Instruction::Sub;
			goto math;
		case TMUL: instr = Instruction::Mul;
			goto math;
		case TDIV: instr = Instruction::SDiv;
			goto math;

		/* TODO comparison */
		case TEQUAL:
			pred = ICmpInst::Predicate::ICMP_EQ;
			goto cmp;
		case TCNE:
			pred = ICmpInst::Predicate::ICMP_NE;
			goto cmp;
		case TCLT:
			pred = ICmpInst::Predicate::ICMP_SLT;
			goto cmp;
		case TCLE:
			pred = ICmpInst::Predicate::ICMP_SLE;
			goto cmp;
		case TCGT:
			pred = ICmpInst::Predicate::ICMP_SGT;
			goto cmp;
		case TCGE:
			pred = ICmpInst::Predicate::ICMP_SGE;
			goto cmp;

		default:
			llvm_unreachable("Error binop used");
			break;

		case TPOW:
			std::vector<Value *> arg;

			Value* lhs_v_d = ctinDouble(lhs_v, context);
			Value* rhs_v_d = ctinDouble(rhs_v, context);
			arg.push_back(lhs_v_d);
			arg.push_back(rhs_v_d);

			Function* fun = context.globalFun.at("pow");
			CallInst* res_D = CallInst::Create(fun, makeArrayRef(arg), "llvm.pow.f64", context.currentBlock());
			//std::cout << "BO:Creating pow call" << endl;
			//std::cout << res_D << endl;
			//std::cout << res_D->getParent() << endl;
			//std::cout << res_D->getParent()->getParent() << endl;
			return ctinInt64(res_D,context);
		}

		return nullptr;
	math:
		return BinaryOperator::Create(instr, lhs_v,rhs_v,
			"", context.currentBlock());
	cmp:
		return ICmpInst::Create(cmpinstr, pred, lhs_v, rhs_v,
			"", context.currentBlock());

	}else {
		Value* lhs_v_d = ctinDouble(lhs_v, context);
		Value* rhs_v_d = ctinDouble(rhs_v, context);
		cmpinstr = Instruction::FCmp;
		switch (op) {
		case TPLUS: instr = Instruction::FAdd;
			goto mathd;
		case TMINUS: instr = Instruction::FSub;
			goto mathd;
		case TMUL: instr = Instruction::FMul;
			goto mathd;
		case TDIV: instr = Instruction::FDiv;
			goto mathd;
			/* TODO comparison */
		case TEQUAL:
			pred = FCmpInst::Predicate::FCMP_OEQ;
			goto cmpd;
		case TCNE:
			pred = FCmpInst::Predicate::FCMP_UNE;
			goto cmpd;
		case TCLT:
			pred = FCmpInst::Predicate::FCMP_OLT;
			goto cmpd;
		case TCLE:
			pred = FCmpInst::Predicate::FCMP_OLE;
			goto cmpd;
		case TCGT:
			pred = FCmpInst::Predicate::FCMP_UGT;
			goto cmpd;
		case TCGE:
			pred = FCmpInst::Predicate::FCMP_UGE;
			goto cmpd;

		default:
			llvm_unreachable("Error binop used");
			break;
		case TPOW:

			std::vector<Value *> arg;
			arg.push_back(lhs_v_d);
			arg.push_back(rhs_v_d);



			Function* fun = context.globalFun.at("pow");
			CallInst* call = CallInst::Create(fun, makeArrayRef(arg), "llvm.pow.f64", context.currentBlock());
			//std::cout << "BO:Creating pow call" << endl;
			//std::cout << call << endl;
			//std::cout << call->getParent() << endl;
			//std::cout << call->getParent()->getParent() << endl;
			return call;
			/* TODO comparison */

		}
	mathd:
		return BinaryOperator::Create(instr, lhs_v_d,
			rhs_v_d, "", context.currentBlock());
	cmpd:
		return FCmpInst::Create(cmpinstr, pred, lhs_v_d, rhs_v_d,
			"", context.currentBlock());
	}

}

Value* NAssignment::codeGen(CodeGenContext& context)
{
	std::cout << "Creating assignment for " << lhs.name << endl;

	Value* loc;
	if (!((loc = context.find_locals(lhs.name)))) {
		std::cerr << "undeclared variable " << lhs.name << endl;
		return nullptr;
	}
	return new StoreInst(rhs.codeGen(context), loc, false, context.currentBlock());
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

Value* NIfBlock::codeGen(CodeGenContext& context)
{

	Function* iff = context.currentBlock()->getParent();
	//std::cout << "If:Creating BasicBlocks:"<< iff << endl;
	BasicBlock* bblock  = BasicBlock::Create(getGlobalContext(), "if", iff);
	BasicBlock* ThenBB  = BasicBlock::Create(getGlobalContext(), "then",  iff);
	BasicBlock* ElseBB  = BasicBlock::Create(getGlobalContext(), "else",  iff);
	BasicBlock* MergeBB = BasicBlock::Create(getGlobalContext(), "merge", iff);
	BranchInst::Create(bblock, context.currentBlock());
	//std::cout << bblock << endl;
	//std::cout << ThenBB << endl;
	//std::cout << ElseBB << endl;

	context.pushBlock(bblock,"if",true);
	Value* vcond = ctinBoolean(cond.codeGen(context), context);
	Type *thenType, *elseType;
	BasicBlock* tmp = BasicBlock::Create(getGlobalContext(), "tmp", context.currentBlock()->getParent());
	std::cout << "START type infer" << endl << endl;
	context.pushBlock(tmp, "tmp", true);
	thenType = thenblock.codeGen(context)->getType();
	elseType = elseblock.codeGen(context)->getType();
	context.popBlock();
	std::cout << "FINISH type infer" << endl << endl;
	tmp->eraseFromParent();
	assert(thenType == elseType && "elseblock and thenblock must have the same type!");
	AllocaInst* alloc = new AllocaInst(elseType, "ifv", context.currentBlock());
	Value *CondInst = new ICmpInst(*context.currentBlock(), ICmpInst::ICMP_NE, vcond , ConstantInt::get(Type::getInt1Ty(getGlobalContext()), 0), "cond");
	BranchInst::Create(ThenBB, ElseBB, CondInst, context.currentBlock());

	Value *thenValue, *elseValue;
	std::cout << "Creating Then" << endl;
	context.pushBlock(ThenBB,"then",true);
	thenValue = thenblock.codeGen(context);
	StoreInst* thenStore = new StoreInst(thenValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(thenStore);
	BranchInst::Create(MergeBB, context.currentBlock());
	context.popBlock();


	std::cout << "Creating Else" << endl;
	context.pushBlock(ElseBB,"else",true);
	elseValue = elseblock.codeGen(context);
	StoreInst* elseStore = new StoreInst(elseValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(elseStore);
	BranchInst::Create(MergeBB, context.currentBlock());
	context.popBlock();

	context.popBlock();

	context.pushBlock(MergeBB, "join", true);
	LoadInst* rtv=new LoadInst(alloc, "", false, context.currentBlock());
	return rtv;
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
	std::cout << "Creating function: " << id.name << endl;
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); ++it) {
		argTypes.push_back(typeOf((**it).type));
	}
	FunctionType* ftype = FunctionType::get(typeOf(type), makeArrayRef(argTypes), false);
	Function* function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
	std::cout << "Fn:Creating BasicBlocks:" << function << endl;
	BasicBlock* bblock = BasicBlock::Create(getGlobalContext(), "entry", function, nullptr);
	//std::cout << bblock << endl;
	//std::cout << bblock->getParent() << endl;
	//std::cout << bblock->getParent()->getParent() << endl;
	context.pushBlock(bblock,"fn:"+ id.name,true);

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
	return function;
}

