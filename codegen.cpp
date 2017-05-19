#include "node.h"
#include "codegen.h"
#include "parser.hpp"
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/raw_os_ostream.h>

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	std::clog << "Generating code...\n";

	/* Create the top level interpreter function to call as entry */
	std::vector<Type*> argTypes;
	auto ftype = FunctionType::get(Type::getInt64Ty(llvmContext), makeArrayRef(argTypes), false);
	mainFunction = Function::Create(ftype, GlobalValue::ExternalLinkage, "main", module);
	auto bblock = BasicBlock::Create(llvmContext, "entry", mainFunction, nullptr);

	/* Push a new variable/block context */
	pushBlock(bblock, "main");
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	if (!getCurrentReturnValue()) {
		ReturnInst::Create(llvmContext, ConstantInt::get(Type::getInt64Ty(llvmContext), 0), currentBlock());
	} else {
		assert(getCurrentReturnValue()->getType()==Type::getInt64Ty(llvmContext)&&"Main must return Int64!");
		ReturnInst::Create(llvmContext, getCurrentReturnValue(), currentBlock());
	}
	popBlock();

	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::clog << "Code is generated.\n";
	PassManager<Module> pm;
	AnalysisManager<Module> am;
	pm.addPass(PrintModulePass(outs()));
	pm.run(*module, am);
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() const
{
	std::clog << "Running code...\n";
	auto ee = EngineBuilder(std::unique_ptr<Module>(module)).create();
	ee->finalizeObject();
	std::vector<GenericValue> noargs;
	auto v = ee->runFunction(mainFunction, noargs);
	std::clog << "Code was run.\n";
	return v;
}

/* Returns an LLVM type based on the identifier */
static Type* typeOf(const NIdentifier& type, CodeGenContext& context)
{
	if (type.name.compare("int") == 0) {
		return Type::getInt64Ty(context.llvmContext);
	}
	if (type.name.compare("double") == 0) {
		return Type::getDoubleTy(context.llvmContext);
	}
	if (type.name.compare("bool") == 0) {
		return Type::getInt1Ty(context.llvmContext);
	}
	return Type::getVoidTy(context.llvmContext);
}

static inline Value* cast(Value* op, Type* dest, CodeGenContext& context)
{
	if (op->getType() == Type::getInt64Ty(context.llvmContext) && dest == Type::getDoubleTy(context.llvmContext)) {
		//Int64 -> Double
		return new SIToFPInst(op, dest, "", context.currentBlock());
	}
	if (op->getType() == Type::getDoubleTy(context.llvmContext) && dest == Type::getInt64Ty(context.llvmContext)) {
		//Double -> Int64
		return new FPToSIInst(op, dest, "", context.currentBlock());
	}
	if (op->getType() == Type::getInt64Ty(context.llvmContext) && dest == Type::getInt1Ty(context.llvmContext)) {
		//Int64 -> Boolean
		return ICmpInst::Create(Instruction::ICmp, ICmpInst::Predicate::ICMP_NE, op, ConstantInt::get(Type::getInt64Ty(context.llvmContext), 0),
		                        "", context.currentBlock());
	}
	if (op->getType() == Type::getDoubleTy(context.llvmContext) && dest == Type::getInt1Ty(context.llvmContext)) {
		//Double -> Boolean
		return FCmpInst::Create(Instruction::FCmp, ICmpInst::Predicate::FCMP_ONE, op, ConstantFP::get(Type::getDoubleTy(context.llvmContext), 0.0),
		                        "", context.currentBlock());
	}
	llvm_unreachable("Invaild cast!");
}

static inline Value* castToIfNeed(Value* op, Type* dest, CodeGenContext& context)
{
	Value* res;
	if (op->getType() != dest) {
		res = cast(op, dest, context);
	} else res = op;
	return res;
}

static inline Value* castDouble(Value* op, CodeGenContext& context)
{
	return castToIfNeed(op, Type::getDoubleTy(context.llvmContext), context);
}

static inline Value* castInt64(Value* op, CodeGenContext& context)
{
	return castToIfNeed(op, Type::getInt64Ty(context.llvmContext), context);
}

static inline Value* castBoolean(Value* op, CodeGenContext& context)
{
	return castToIfNeed(op, Type::getInt1Ty(context.llvmContext), context);
}

int64_t NBool::getValue() 
{
	return static_cast<int64_t>(value);
}

int64_t NInteger::getValue() 
{
	return value;
}

/* -- Code Generation -- */

Value* NBool::codeGen(CodeGenContext& context)
{
	std::clog << "Creating boolean: " << value << std::endl;
	return ConstantInt::get(Type::getInt1Ty(context.llvmContext), value, true);
}

Value* NInteger::codeGen(CodeGenContext& context)
{
	std::clog << "Creating integer: " << value << std::endl;
	return ConstantInt::get(Type::getInt64Ty(context.llvmContext), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context)
{
	std::clog << "Creating double: " << value << std::endl;
	return ConstantFP::get(Type::getDoubleTy(context.llvmContext), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context)
{
	std::clog << "Creating identifier reference: " << name << std::endl;
	Value* loc;
	if (!((loc = context.find_locals(name)))) {
		std::cerr << "undeclared variable " << name << std::endl;
		return nullptr;
	}
	return new LoadInst(loc, "", false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context)
{
	auto function = context.module->getFunction(id.name.c_str());
	if (function == nullptr) {
		auto loc = context.find_locals(context.ftrace() + "__fn_" + id.name);
		if (loc)std::clog << "finding local function: " << id.name << loc->getType()->isFunctionTy() << std::endl;
		if (loc) {
			function = static_cast<Function*>(loc);
		} else {
			std::cerr << "no such function " << id.name << std::endl;
			assert(false);
		}
	}
	std::clog << "Creating method call: " << id.name << std::endl;
	std::vector<Value*> args;
	auto i = 0;
	for (ExpressionList::const_iterator it = arguments.begin(); it != arguments.end(); ++it) {
		std::clog << "codeGen arg" << i << std::endl;
		args.push_back((**it).codeGen(context));
		std::clog << "arg" << i++ << "'s type: ";
		args.back()->getType()->print(context.llclog);
		(context.llclog << "\n").flush();
	}
	for (auto ex: context.extra[context.ftrace() + "__fn_" + id.name]) {
		std::clog << "extra" << i << std::endl;
		auto val = context.find_locals(ex);
		std::clog << "arg" << i++ << "'s type: ";
		val->getType()->print(context.llclog);
		(context.llclog << "\n").flush();
		args.push_back(val);
	}
	auto call = CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());

	return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
	std::clog << "Creating binary operation " << op << std::endl;
	Instruction::BinaryOps instr;
	Instruction::OtherOps cmpinstr;
	ICmpInst::Predicate pred;
	auto lhs_v = lhs.codeGen(context);
	auto rhs_v = rhs.codeGen(context);
	context.llclog << "The operands' types are ";
	lhs_v->getType()->print(context.llclog);
	context.llclog << " and ";
	rhs_v->getType()->print(context.llclog);
	(context.llclog << "\n").flush();
	//Int64 operands
	if (lhs_v->getType() == Type::getInt64Ty(context.llvmContext) && rhs_v->getType() == Type::getInt64Ty(context.llvmContext)) {
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

		case TPOW:
			std::vector<Value *> arg;

			auto lhs_v_d = castDouble(lhs_v, context);
			auto rhs_v_d = castDouble(rhs_v, context);
			arg.push_back(lhs_v_d);
			arg.push_back(rhs_v_d);

			auto fun = context.globalFun.at("pow");
			auto res_D = CallInst::Create(fun, makeArrayRef(arg), "llvm.pow.f64", context.currentBlock());
			return castInt64(res_D, context);
		}

	math:
		return BinaryOperator::Create(instr, lhs_v, rhs_v,
		                              "", context.currentBlock());
	cmp:
		return ICmpInst::Create(cmpinstr, pred, lhs_v, rhs_v,
		                        "", context.currentBlock());

	}
	if (lhs_v->getType() == Type::getDoubleTy(context.llvmContext) || rhs_v->getType() == Type::getDoubleTy(context.llvmContext)) {
		auto lhs_v_d = castDouble(lhs_v, context);
		auto rhs_v_d = castDouble(rhs_v, context);
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
		case TPOW:

			std::vector<Value *> arg;
			arg.push_back(lhs_v_d);
			arg.push_back(rhs_v_d);
			auto fun = context.globalFun.at("pow");
			auto call = CallInst::Create(fun, makeArrayRef(arg), "llvm.pow.f64", context.currentBlock());

			return call;

		}
	mathd:
		return BinaryOperator::Create(instr, lhs_v_d,
		                              rhs_v_d, "", context.currentBlock());
	cmpd:
		return FCmpInst::Create(cmpinstr, pred, lhs_v_d, rhs_v_d,
		                        "", context.currentBlock());
	}
	assert(op == TNOT);
	return ConstantInt::get(Type::getInt1Ty(context.llvmContext), !(static_cast<NValue&>(rhs).getValue()));
}

Value* NAssignment::codeGen(CodeGenContext& context)
{
	std::clog << "Creating assignment for " << lhs.name << std::endl;

	Value* loc;
	if (!((loc = context.find_locals(lhs.name)))) {
		std::cerr << "undeclared variable " << lhs.name << std::endl;
		assert(false);
	}
	auto val = rhs.codeGen(context);
	return new StoreInst(val, loc, false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context)
{
	Value* last = nullptr;
	for (StatementList::const_iterator it = statements.begin(); it != statements.end(); ++it) {
		std::clog << "Generating code for " << typeid(**it).name() << std::endl;
		last = (**it).codeGen(context);
	}
	std::clog << "Creating block" << std::endl;
	return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	std::clog << "Generating code for " << typeid(expression).name() << std::endl;
	return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	std::clog << "Generating return code for " << typeid(expression).name() << std::endl;
	auto returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NIfBlock::codeGen(CodeGenContext& context)
{
	std::vector<Type*> argTypes;
	VariableList::const_iterator it;
	auto ftype = FunctionType::get(Type::getVoidTy(context.llvmContext), makeArrayRef(argTypes), false);
	auto fake = Function::Create(ftype, GlobalValue::InternalLinkage, "", context.module);
	auto iff = context.currentBlock()->getParent();
	auto bblock = BasicBlock::Create(context.llvmContext, "if_" + context.trace(), iff);
	auto ThenBB = BasicBlock::Create(context.llvmContext, "then_" + context.trace(), iff);
	auto ElseBB = BasicBlock::Create(context.llvmContext, "else_" + context.trace(), iff);
	auto MergeBB = BasicBlock::Create(context.llvmContext, "merge_" + context.trace(), iff);
	BranchInst::Create(bblock, context.currentBlock());

	context.pushBlock(bblock, "if", true);
	auto vcond = castBoolean(cond.codeGen(context), context);
	auto tmp = BasicBlock::Create(context.llvmContext, "tmp_" + context.trace(), fake);
	std::clog << "START type infer" << std::endl;
	context.pushBlock(tmp, "tmp", true);
	auto thenType = thenblock.codeGen(context)->getType();
	auto elseType = elseblock.codeGen(context)->getType();
	context.popBlockUntil(tmp);
	context.popBlock();
	tmp->eraseFromParent();
	fake->eraseFromParent();
	std::clog << "FINISH type infer" << std::endl;
	std::clog << "then Type" << std::endl;
	thenType->print(context.llclog);
	(context.llclog << "\n").flush();
	std::clog << "else Type" << std::endl;
	elseType->print(context.llclog);
	(context.llclog << "\n").flush();
	std::clog << std::endl;
	assert(thenType == elseType && "elseblock and thenblock must have the same type!");
	auto alloc = new AllocaInst(elseType, "ifv", context.currentBlock());
	auto CondInst = new ICmpInst(*context.currentBlock(), ICmpInst::ICMP_NE, vcond, ConstantInt::get(Type::getInt1Ty(context.llvmContext), 0), "cond");
	BranchInst::Create(ThenBB, ElseBB, CondInst, context.currentBlock());

	std::clog << "Creating Then" << std::endl;
	context.pushBlock(ThenBB, "then", true);
	auto thenValue = thenblock.codeGen(context);
	auto thenStore = new StoreInst(thenValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(thenStore);
	BranchInst::Create(MergeBB, context.currentBlock());
	context.popBlockUntil(ThenBB);
	context.popBlock();


	std::clog << "Creating Else" << std::endl;
	context.pushBlock(ElseBB, "else", true);
	auto elseValue = elseblock.codeGen(context);
	auto elseStore = new StoreInst(elseValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(elseStore);
	BranchInst::Create(MergeBB, context.currentBlock());
	context.popBlockUntil(ElseBB);
	context.popBlock();
	context.popBlockUntil(bblock);
	context.popBlock();

	context.pushBlock(MergeBB, "join", true);
	return new LoadInst(alloc, "", false, context.currentBlock());
}

Value* NWhileBlock::codeGen(CodeGenContext& context)
{
	auto iff = context.currentBlock()->getParent();
	auto bblock = BasicBlock::Create(context.llvmContext, "while_" + context.trace(), iff);
	auto then_bb = BasicBlock::Create(context.llvmContext, "do_" + context.trace(), iff);
	auto merge_bb = BasicBlock::Create(context.llvmContext, "join_" + context.trace(), iff);
	BranchInst::Create(bblock, context.currentBlock());

	context.pushBlock(bblock, "while", true);
	auto condv = cond.codeGen(context);
	auto vcond = castBoolean(condv, context);


	Value* CondInst = new ICmpInst(*context.currentBlock(), ICmpInst::ICMP_NE, vcond, ConstantInt::get(Type::getInt1Ty(context.llvmContext), 0), "cond");
	BranchInst::Create(then_bb, merge_bb, CondInst, context.currentBlock());

	std::clog << "Creating While" << std::endl;
	context.pushBlock(then_bb, "do", true);
	doblock.codeGen(context);
	BranchInst::Create(bblock, context.currentBlock());
	context.popBlockUntil(then_bb);
	context.popBlock();
	context.popBlockUntil(bblock);
	context.popBlock();

	context.pushBlock(merge_bb, "join", true);
	return condv;
}

Value* NVariableDefinition::codeGen(CodeGenContext& context)
{
	std::clog << "Creating variable declaration " << type.name << " " << id.name << std::endl;
	auto alloc = new AllocaInst(typeOf(type, context), id.name.c_str(), context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != nullptr) {
		NAssignment assn(id, *assignmentExpr);
		assn.codeGen(context);
	}
	return alloc;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context)
{
	std::vector<Type*> argTypes;
	for (VariableList::const_iterator it = arguments.begin(); it != arguments.end(); ++it) {
		argTypes.push_back(typeOf((**it).type, context));
	}
	FunctionType* ftype = FunctionType::get(typeOf(type, context), makeArrayRef(argTypes), false);
	Function* function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.module);
	return function;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	std::clog << "Creating function: " << id.name << std::endl;
	std::vector<Type*> argTypes;

	for (auto it = arguments.begin(); it != arguments.end(); ++it) {
		argTypes.push_back(typeOf((**it).type, context));
	}
	FunctionType* ftype = FunctionType::get(typeOf(type, context), makeArrayRef(argTypes), false);
	Function* function = local ?
		                     Function::Create(ftype, GlobalValue::PrivateLinkage, context.ftrace() + "__fn_" + id.name, context.module)
		                     : Function::Create(ftype, GlobalValue::InternalLinkage, id.name, context.module);
	if (local) {
		context.locals()[context.ftrace() + "__fn_" + id.name] = function;
	}
	std::clog << "Fn:Creating BasicBlocks:" << function << std::endl;
	auto bblock = BasicBlock::Create(context.llvmContext, "entry", function, nullptr);
	context.pushBlock(bblock, "fn_" + id.name, local, true);
	context.funcBlocks.emplace_back("fn_" + id.name);
	auto argsValues = function->arg_begin();
	std::vector<Value*> storeInst;
	for (auto it = arguments.begin(); it != arguments.end(); ++it) {
		(**it).codeGen(context);
		Value* argumentValue = &(*argsValues++);
		argumentValue->setName((*it)->id.name);
		auto inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
		storeInst.push_back(inst);
	}
	block.codeGen(context);
	if (local && context.extra[context.ftrace(1) + "__fn_" + id.name].size() != 0) {
		std::clog << "Fn:Recreating local function with extra" << id.name << std::endl;
		context.popBlockUntil(bblock);
		context.popBlock();
		context.funcBlocks.pop_back();
		bblock->eraseFromParent();
		function->eraseFromParent();
		for (auto ex: context.extra[context.ftrace() + "__fn_" + id.name]) {
			argTypes.push_back(context.find_locals(ex)->getType());
		}
		ftype = FunctionType::get(typeOf(type, context), makeArrayRef(argTypes), false);
		function = Function::Create(ftype, GlobalValue::PrivateLinkage, context.ftrace() + "__fn_" + id.name, context.module);
		context.locals()[context.ftrace() + "__fn_" + id.name] = function;
		bblock = BasicBlock::Create(context.llvmContext, "entry", function, nullptr);
		context.pushBlock(bblock, context.ftrace() + "__fn_" + id.name, local, true);
		context.funcBlocks.emplace_back("fn_" + id.name);

		argsValues = function->arg_begin();
		std::clog << "Arg " << function->arg_size() << std::endl;
		for (auto it = arguments.begin(); it != arguments.end(); ++it) {
			std::clog << "Setting arg " << (*it)->id.name << std::endl;
			(*it)->codeGen(context);
			Value* argumentValue = &(*argsValues++);

			argumentValue->setName((*it)->id.name);
			auto inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
			storeInst.push_back(inst);
		}
		for (auto ex: context.extra[context.ftrace(1) + "__fn_" + id.name]) {
			std::clog << "Setting extra arg " << ex << std::endl;
			if (argsValues == function->arg_end()) {
				assert(false);
			}
			Value* argumentPointer = &(*argsValues++);
			argumentPointer->setName(context.ftrace() + ex);
			context.locals()[ex] = argumentPointer;
		}
		block.codeGen(context);
	}
	ReturnInst::Create(context.llvmContext, context.getCurrentReturnValue(), context.currentBlock());
	context.popBlockUntil(bblock);
	context.popBlock();
	context.funcBlocks.pop_back();
	return function;
}
