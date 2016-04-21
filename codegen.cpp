#include "node.h"
#include "codegen.h"
#include "parser.hpp"

using namespace std;

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	std::clog << "Generating code...\n";

	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	FunctionType* ftype = FunctionType::get(Type::getInt64Ty(TheContext), makeArrayRef(argTypes), false);
	mainFunction = Function::Create(ftype, GlobalValue::ExternalLinkage, "main", module);
	BasicBlock* bblock = BasicBlock::Create(TheContext, "entry", mainFunction, nullptr);

	/* Push a new variable/block context */
	pushBlock(bblock,"main");
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	if(!getCurrentReturnValue()) {
		ReturnInst::Create(TheContext, ConstantInt::get(Type::getInt64Ty(TheContext), 0), currentBlock());
	}
	else {
		assert(getCurrentReturnValue()->getType()==Type::getInt64Ty(TheContext)&&"Main must return Int64!");
		ReturnInst::Create(TheContext, getCurrentReturnValue(), currentBlock());
	}
	popBlock();

	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::clog << "Code is generated.\n";
	PassManager<Module> pm;
	AnalysisManager<Module>* am = new AnalysisManager<Module>;
	pm.addPass(PrintModulePass(outs()));
	pm.run(*module,*am);
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() const
{
	std::clog << "Running code...\n";
	ExecutionEngine* ee = EngineBuilder(unique_ptr<Module>(module)).create();
	ee->finalizeObject();
	vector<GenericValue> noargs;
	GenericValue v = ee->runFunction(mainFunction, noargs);
	std::clog << "Code was run.\n";
	return v;
}

/* Returns an LLVM type based on the identifier */
static Type* typeOf(const NIdentifier& type, CodeGenContext& context)
{
	if (type.name.compare("int") == 0) {
		return Type::getInt64Ty(context.TheContext);
	}
	else if (type.name.compare("double") == 0) {
		return Type::getDoubleTy(context.TheContext);
	}
	else if (type.name.compare("bool") == 0) {
		return Type::getInt1Ty(context.TheContext);
	}
	return Type::getVoidTy(context.TheContext);
}

static inline Value* cast(Value* op, Type* dest, CodeGenContext& context)
{
	if (op->getType() == Type::getInt64Ty(context.TheContext) &&dest == Type::getDoubleTy(context.TheContext)) {
		//Int64 -> Double
		return new SIToFPInst(op, dest, "", context.currentBlock());
	} else if (op->getType() == Type::getDoubleTy(context.TheContext) && dest == Type::getInt64Ty(context.TheContext)) {
		//Double -> Int64
		return new FPToSIInst(op, dest, "", context.currentBlock());
	} else if (op->getType() == Type::getInt64Ty(context.TheContext) && dest == Type::getInt1Ty(context.TheContext)) {
		//Int64 -> Boolean
		return ICmpInst::Create(Instruction::ICmp, ICmpInst::Predicate::ICMP_NE, op, ConstantInt::get(Type::getInt64Ty(context.TheContext),0),
			"", context.currentBlock());
	} else if (op->getType() == Type::getDoubleTy(context.TheContext) && dest == Type::getInt1Ty(context.TheContext)) {
		//Double -> Boolean
		return FCmpInst::Create(Instruction::FCmp, ICmpInst::Predicate::FCMP_ONE, op, ConstantFP::get(Type::getDoubleTy(context.TheContext), 0.0),
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
	return castToIfNeed(op, Type::getDoubleTy(context.TheContext),context);
}

static inline Value* ctinInt64(Value* op, CodeGenContext& context)
{
	return castToIfNeed(op, Type::getInt64Ty(context.TheContext), context);
}

static inline Value* ctinBoolean(Value* op, CodeGenContext& context)
{
	return castToIfNeed(op, Type::getInt1Ty(context.TheContext), context);
}
/* -- Code Generation -- */

Value* NBool::codeGen(CodeGenContext& context)
{
	std::clog << "Creating boolean: " << value << endl;
	return ConstantInt::get(Type::getInt1Ty(context.TheContext), value, true);
}

Value* NInteger::codeGen(CodeGenContext& context)
{
	std::clog << "Creating integer: " << value << endl;
	return ConstantInt::get(Type::getInt64Ty(context.TheContext), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context)
{
	std::clog << "Creating double: " << value << endl;
	return ConstantFP::get(Type::getDoubleTy(context.TheContext), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context)
{
	std::clog << "Creating identifier reference: " << name << endl;
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
	std::clog << "Creating method call: " << id.name << endl;
	std::vector<Value*> args;
	ExpressionList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); ++it) {

		args.push_back((**it).codeGen(context));
		std::clog << "    arg of type " << args.back()->getType() << endl;
	}
	CallInst* call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());

	return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
	std::clog << "Creating binary operation " << op << endl;
	Instruction::BinaryOps instr;
	Instruction::OtherOps cmpinstr;
	ICmpInst::Predicate pred;
	Value* lhs_v = lhs.codeGen(context);
	Value* rhs_v = rhs.codeGen(context);
	std::clog << "The operands' types are " << lhs_v->getType()<< " and " << rhs_v->getType() << endl;
	if (lhs_v->getType() == Type::getInt64Ty(context.TheContext) && rhs_v->getType() == Type::getInt64Ty(context.TheContext)) {
		cmpinstr = Instruction::ICmp;
		switch (op) {
		case TPLUS:
			instr = Instruction::Add;
			goto math;
		case TMINUS:
			instr = Instruction::Sub;
			goto math;
		case TMUL:
			instr = Instruction::Mul;
			goto math;
		case TDIV:
			instr = Instruction::SDiv;
			goto math;

		case TCEQ:
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
		case TPLUS: 
			instr = Instruction::FAdd;
			goto mathd;
		case TMINUS:
			instr = Instruction::FSub;
			goto mathd;
		case TMUL:
			instr = Instruction::FMul;
			goto mathd;
		case TDIV:
			instr = Instruction::FDiv;
			goto mathd;

		case TCEQ:
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

			return call;

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
	std::clog << "Creating assignment for " << lhs.name << endl;

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
		std::clog << "Generating code for " << typeid(**it).name() << endl;
		last = (**it).codeGen(context);
	}
	std::clog << "Creating block" << endl;
	return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	std::clog << "Generating code for " << typeid(expression).name() << endl;
	return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	std::clog << "Generating return code for " << typeid(expression).name() << endl;
	Value* returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NIfBlock::codeGen(CodeGenContext& context)
{
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	FunctionType* ftype = FunctionType::get(Type::getVoidTy(context.TheContext), makeArrayRef(argTypes), false);
	Function* fake = Function::Create(ftype, GlobalValue::InternalLinkage, "", context.module);
	Function* iff = context.currentBlock()->getParent();
	BasicBlock* bblock  = BasicBlock::Create(context.TheContext, "if_"+context.trace(), iff);
	BasicBlock* ThenBB  = BasicBlock::Create(context.TheContext, "then_"+context.trace(),  iff);
	BasicBlock* ElseBB  = BasicBlock::Create(context.TheContext, "else_"+context.trace(),  iff);
	BasicBlock* MergeBB = BasicBlock::Create(context.TheContext, "merge_"+context.trace(), iff);
	BranchInst::Create(bblock, context.currentBlock());

	context.pushBlock(bblock,"if",true);
	Value* vcond = ctinBoolean(cond.codeGen(context), context);
	Type *thenType, *elseType;
	BasicBlock* tmp = BasicBlock::Create(context.TheContext, "tmp_", fake);
	std::clog << "START type infer" << endl << endl;
	context.pushBlock(tmp, "tmp", true);
	thenType = thenblock.codeGen(context)->getType();
	elseType = elseblock.codeGen(context)->getType();
	context.popBlockUntil(tmp);
	context.popBlock();
	tmp->eraseFromParent();
	fake->eraseFromParent();
	std::clog << "FINISH type infer" << endl << endl;
	
	assert(thenType == elseType && "elseblock and thenblock must have the same type!");
	AllocaInst* alloc = new AllocaInst(elseType, "ifv", context.currentBlock());
	Value *CondInst = new ICmpInst(*context.currentBlock(), ICmpInst::ICMP_NE, vcond , ConstantInt::get(Type::getInt1Ty(context.TheContext), 0), "cond");
	BranchInst::Create(ThenBB, ElseBB, CondInst, context.currentBlock());

	Value *thenValue, *elseValue;
	std::clog << "Creating Then" << endl;
	context.pushBlock(ThenBB,"then",true);
	thenValue = thenblock.codeGen(context);
	StoreInst* thenStore = new StoreInst(thenValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(thenStore);
	BranchInst::Create(MergeBB, context.currentBlock());
	context.popBlockUntil(ThenBB);
	context.popBlock();


	std::clog << "Creating Else" << endl;
	context.pushBlock(ElseBB,"else",true);
	elseValue = elseblock.codeGen(context);
	StoreInst* elseStore = new StoreInst(elseValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(elseStore);
	BranchInst::Create(MergeBB, context.currentBlock());
	context.popBlockUntil(ElseBB);
	context.popBlock();
	context.popBlockUntil(bblock);
	context.popBlock();

	context.pushBlock(MergeBB, "join", true);
	LoadInst* rtv=new LoadInst(alloc, "", false, context.currentBlock());
	return rtv;
}
Value* NWhileBlock::codeGen(CodeGenContext& context)
{

	Function* iff = context.currentBlock()->getParent();
	BasicBlock* bblock = BasicBlock::Create(context.TheContext, "while_" + context.trace(), iff);
	BasicBlock* ThenBB = BasicBlock::Create(context.TheContext, "do_" + context.trace(), iff);
	BasicBlock* MergeBB = BasicBlock::Create(context.TheContext, "join_" + context.trace(), iff);
	BranchInst::Create(bblock, context.currentBlock());

	context.pushBlock(bblock, "while", true);
	Value* condv= cond.codeGen(context);
	Value* vcond = ctinBoolean(condv, context);


	Value *CondInst = new ICmpInst(*context.currentBlock(), ICmpInst::ICMP_NE, vcond, ConstantInt::get(Type::getInt1Ty(context.TheContext), 0), "cond");
	BranchInst::Create(ThenBB, MergeBB, CondInst, context.currentBlock());

	std::clog << "Creating While" << endl;
	context.pushBlock(ThenBB, "do", true);
	doblock.codeGen(context);
	BranchInst::Create(bblock, context.currentBlock());
	context.popBlockUntil(ThenBB);
	context.popBlock();
	context.popBlockUntil(bblock);
	context.popBlock();

	context.pushBlock(MergeBB, "join", true);
	return condv;
}
Value* NVariableDefinition::codeGen(CodeGenContext& context)
{
	std::clog << "Creating variable declaration " << type.name << " " << id.name << endl;
	AllocaInst* alloc = new AllocaInst(typeOf(type,context), id.name.c_str(), context.currentBlock());
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
		argTypes.push_back(typeOf((**it).type,context));
	}
	FunctionType* ftype = FunctionType::get(typeOf(type,context), makeArrayRef(argTypes), false);
	Function* function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.module);
	return function;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	std::clog << "Creating function: " << id.name << endl;
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); ++it) {
		argTypes.push_back(typeOf((**it).type,context));
	}
	FunctionType* ftype = FunctionType::get(typeOf(type,context), makeArrayRef(argTypes), false);
	Function* function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
	std::clog << "Fn:Creating BasicBlocks:" << function << endl;
	BasicBlock* bblock = BasicBlock::Create(context.TheContext, "entry", function, nullptr);
	context.pushBlock(bblock,"fn_"+ id.name,false);

	Function::arg_iterator argsValues = function->arg_begin();
	Value* argumentValue;
	std::vector<Value*> storeInst;
	for (it = arguments.begin(); it != arguments.end(); ++it) {
		(**it).codeGen(context);

		argumentValue = argsValues.getNodePtrUnchecked();
		argumentValue->setName((*it)->id.name.c_str());
		StoreInst* inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
		storeInst.push_back(inst);
	}

	block.codeGen(context);
	ReturnInst::Create(context.TheContext, context.getCurrentReturnValue(), context.currentBlock());
	context.popTBlock();
	context.popBlock();
	return function;
}

