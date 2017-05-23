#include "node.h"
#include "codegen.h"
#include "parser.hpp"
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/Support/raw_os_ostream.h>
#include <set>

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	dclog << debug_stream::info << "Generating code..." << std::endl;

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
		if (getCurrentReturnValue()->getType() != Type::getInt64Ty(llvmContext)) {
			std::cerr << "Main must return Int64!" << std::endl;
			exit(0);
		}
		ReturnInst::Create(llvmContext, getCurrentReturnValue(), currentBlock());
	}
	popBlockUntil(bblock);
	popBlock();

	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	dclog << debug_stream::info << "Code is generated." << std::endl;
	PassManager<Module> pm;
	AnalysisManager<Module> am;
	pm.addPass(PrintModulePass(outs()));
	pm.run(*module, am);
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode()
{
	dclog << debug_stream::info << "Running code..." << std::endl;
	auto ee = EngineBuilder(std::unique_ptr<Module>(module)).setOptLevel(CodeGenOpt::Aggressive).create();
	ee->finalizeObject();
	std::vector<GenericValue> noargs;
	auto v = ee->runFunction(mainFunction, noargs);
	dclog << debug_stream::info << "Code was run." << std::endl;
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
	context.dclog << debug_stream::info << "Creating boolean: " << value << std::endl;
	return ConstantInt::get(Type::getInt1Ty(context.llvmContext), value, true);
}

Value* NInteger::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Creating integer: " << value << std::endl;
	return ConstantInt::get(Type::getInt64Ty(context.llvmContext), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Creating double: " << value << std::endl;
	return ConstantFP::get(Type::getDoubleTy(context.llvmContext), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Creating identifier reference: " << name << std::endl;
	Value* loc;
	if (!((loc = context.find_locals(name)))) {
		std::cerr << "undeclared variable " << name << std::endl;
		exit(1);
	}
	return new LoadInst(loc, "", false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context)
{
	auto function = context.module->getFunction(id.name.c_str());
	if (function == nullptr) {
		auto loc = context.find_locals(context.ftrace() + "__fn_" + id.name);
		if (loc) context.dclog << debug_stream::info << "Finding local function: " << id.name << std::endl;
		if (loc) {
			function = static_cast<Function*>(loc);
		} else {
			std::cerr << "No such function " << id.name << std::endl;
			exit(1);
		}
	}
	context.dclog << "Creating method call: " << id.name << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	std::vector<Value*> args;
	auto i = 0;
	for (ExpressionList::const_iterator it = arguments.begin(); it != arguments.end(); ++it) {
		context.dclog << "Generating code for arg" << i << std::endl;
		context.dclog << debug_stream::indent(2, +1);
		args.push_back((*it)->codeGen(context));
		context.dclog << debug_stream::indent(2, -1);
		context.dclog << debug_stream::info << "arg" << i++ << "'s type: " << std::flush;
		if (context.dclog.max_level >= debug_stream::info) {
			args.back()->getType()->print(context.llclog);
			(context.llclog << "\n").flush();
		}
	}
	for (auto ex : context.extra[context.ftrace() + "__fn_" + id.name]) {
		context.dclog << "Generating code for extra" << i << std::endl;
		auto val = context.find_locals(ex);
		context.dclog << "arg" << i++ << "'s type: " << std::flush;
		if (context.dclog.max_level >= debug_stream::info) {
			val->getType()->print(context.llclog);
			(context.llclog << "\n").flush();
		}
		args.push_back(val);
	}
	context.dclog << debug_stream::indent(2, -1);

	return CallInst::Create(function, makeArrayRef(args), "", context.currentBlock());
}

Value* NBinaryOperator::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Creating binary operation " << op << std::endl;
	Instruction::BinaryOps instr;
	Instruction::OtherOps cmpinstr;
	ICmpInst::Predicate pred;
	context.dclog << "Generating lhs" << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	auto lhs_v = lhs.codeGen(context);
	context.dclog << debug_stream::info << debug_stream::indent(2, -1);
	context.dclog << "Generating rhs" << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	auto rhs_v = rhs.codeGen(context);
	context.dclog << debug_stream::info << debug_stream::indent(2, -1) << "The operands' types are " << std::flush;
	if (context.dclog.max_level >= debug_stream::info) {
		lhs_v->getType()->print(context.llclog);
		(context.llclog << " and ").flush();
		rhs_v->getType()->print(context.llclog);
		(context.llclog << "\n").flush();
	}
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

			return CallInst::Create(fun, makeArrayRef(arg), "llvm.pow.f64", context.currentBlock());

		}
	mathd:
		return BinaryOperator::Create(instr, lhs_v_d,
		                              rhs_v_d, "", context.currentBlock());
	cmpd:
		return FCmpInst::Create(cmpinstr, pred, lhs_v_d, rhs_v_d,
		                        "", context.currentBlock());
	}
	if (op != TNOT) {
		std::cerr << "Error binop used" << std::endl;
		exit(0);
	}
	return ConstantInt::get(Type::getInt1Ty(context.llvmContext), !(static_cast<NValue&>(rhs).getValue()));
}

Value* NAssignment::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Creating assignment for " << lhs.name << std::endl;

	Value* loc;
	if (!((loc = context.find_locals(lhs.name)))) {
		std::cerr << "undeclared variable " << lhs.name << std::endl;
		exit(1);
	}
	auto val = rhs.codeGen(context);
	return new StoreInst(val, loc, false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context)
{
	Value* last = nullptr;
	context.dclog << debug_stream::info << "Creating block " << this << std::endl;
	context.dclog << debug_stream::indent(1, +1);
	for (StatementList::const_iterator it = statements.begin(); it != statements.end(); ++it) {
		context.dclog << debug_stream::info << "Generating code with " << typeid(**it).name() << " in " << this << std::endl;
		context.dclog << debug_stream::indent(1, +1);
		last = (*it)->codeGen(context);
		context.dclog << debug_stream::indent(1, -1);
	}
	context.dclog << debug_stream::indent(1, -1);
	context.dclog << debug_stream::info << "-Created block " << this << std::endl;
	return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	context.dclog << "Generating code for " << typeid(expression).name() << std::endl;
	return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Generating return code in " << this << " with " << typeid(expression).name() << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	auto returnValue = expression.codeGen(context);
	context.dclog << debug_stream::indent(2, -1);
	context.dclog << debug_stream::info << "-Generated return code " << this << std::endl;

	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NIfBlock::codeGen(CodeGenContext& context)
{
	std::vector<Type*> argTypes;
	VariableList::const_iterator it;
	context.dclog << debug_stream::info << "Creating if " << this << std::endl;
	context.dclog << debug_stream::indent(1, +1);

	auto ftype = FunctionType::get(Type::getVoidTy(context.llvmContext), makeArrayRef(argTypes), false);
	auto fake = Function::Create(ftype, GlobalValue::InternalLinkage, "", context.module);
	auto iff = context.currentBlock()->getParent();
	auto bblock = BasicBlock::Create(context.llvmContext, context.trace() + "if", iff);
	auto then_bb = BasicBlock::Create(context.llvmContext, context.trace() + "then", iff);
	auto else_bb = BasicBlock::Create(context.llvmContext, context.trace() + "else", iff);
	auto merge_bb = BasicBlock::Create(context.llvmContext, context.trace() + "merge", iff);
	BranchInst::Create(bblock, context.currentBlock());

	context.pushBlock(bblock, "if", true);
	context.dclog << debug_stream::info << "Generating if condition in " << this << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	auto vcond = castBoolean(cond.codeGen(context), context);
	context.dclog << debug_stream::indent(2, -1);
	context.dclog << debug_stream::info << "-Generated if condition in " << this << std::endl;


	auto tmp = BasicBlock::Create(context.llvmContext, "tmp_" + context.trace(), fake);
	context.dclog << debug_stream::info << "Start type infer in " << this << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	context.pushBlock(tmp, "tmp", true);
	auto thenType = thenblock.codeGen(context)->getType();
	auto elseType = elseblock.codeGen(context)->getType();
	context.popBlockUntil(tmp);
	context.popBlock();
	tmp->eraseFromParent();
	fake->eraseFromParent();
	context.dclog << debug_stream::indent(2, -1);
	context.dclog << debug_stream::info << "-Finish type infer in " << this << std::endl;
	if (context.dclog.max_level >= debug_stream::info) {
		context.dclog << "type of then block: " << std::flush;
		thenType->print(context.llclog);
		(context.llclog << "\n").flush();
		context.dclog << "type of else block: " << std::flush;
		elseType->print(context.llclog);
		(context.llclog << "\n").flush();
	}
	if (thenType != elseType) {
		std::cerr << "elseblock and thenblock must have the same type!" << std::endl;
		exit(0);
	}
	auto alloc = new AllocaInst(elseType, context.module->getDataLayout().getAllocaAddrSpace(), "ifv", context.currentBlock());
	auto CondInst = new ICmpInst(*context.currentBlock(), ICmpInst::ICMP_NE, vcond, ConstantInt::get(Type::getInt1Ty(context.llvmContext), 0), "cond");
	BranchInst::Create(then_bb, else_bb, CondInst, context.currentBlock());

	context.dclog << debug_stream::info << "Creating then block in " << this << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	context.pushBlock(then_bb, "then", true);
	auto thenValue = thenblock.codeGen(context);
	auto thenStore = new StoreInst(thenValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(thenStore);
	BranchInst::Create(merge_bb, context.currentBlock());
	context.popBlockUntil(then_bb);
	context.popBlock();
	context.dclog << debug_stream::indent(2, -1);
	context.dclog << debug_stream::info << "-Created then block in " << this << std::endl;

	context.dclog << debug_stream::info << "Creating else block " << this << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	context.pushBlock(else_bb, "else", true);
	auto elseValue = elseblock.codeGen(context);
	auto elseStore = new StoreInst(elseValue, alloc, false, context.currentBlock());
	context.setCurrentReturnValue(elseStore);
	BranchInst::Create(merge_bb, context.currentBlock());
	context.popBlockUntil(else_bb);
	context.popBlock();
	context.dclog << debug_stream::indent(2, -1);
	context.dclog << debug_stream::info << "-Created else block in " << this << std::endl;


	context.popBlockUntil(bblock);
	context.popBlock();
	context.dclog << debug_stream::indent(1, -1);
	context.pushBlock(merge_bb, "merge", true);
	context.dclog << debug_stream::info << "-Created if " << this << std::endl;
	return new LoadInst(alloc, "", false, context.currentBlock());
}

Value* NWhileBlock::codeGen(CodeGenContext& context)
{
	auto iff = context.currentBlock()->getParent();
	auto bblock = BasicBlock::Create(context.llvmContext, context.trace() + "while", iff);
	auto then_bb = BasicBlock::Create(context.llvmContext, context.trace() + "do", iff);
	auto merge_bb = BasicBlock::Create(context.llvmContext, context.trace() + "join", iff);
	BranchInst::Create(bblock, context.currentBlock());

	context.pushBlock(bblock, "while", true);
	auto condv = cond.codeGen(context);
	auto vcond = castBoolean(condv, context);


	Value* CondInst = new ICmpInst(*context.currentBlock(), ICmpInst::ICMP_NE, vcond, ConstantInt::get(Type::getInt1Ty(context.llvmContext), 0), "cond");
	BranchInst::Create(then_bb, merge_bb, CondInst, context.currentBlock());

	context.dclog << debug_stream::info << "Creating while" << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	context.pushBlock(then_bb, "do", true);
	doblock.codeGen(context);
	BranchInst::Create(bblock, context.currentBlock());
	context.popBlockUntil(then_bb);
	context.popBlock();
	context.popBlockUntil(bblock);
	context.popBlock();
	context.dclog << debug_stream::indent(2, -1);

	context.pushBlock(merge_bb, "join", true);
	return condv;
}

Value* NVariableDefinition::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Creating variable declaration " << type.name << " " << id.name << std::endl;
	auto alloc = new AllocaInst(typeOf(type, context), context.module->getDataLayout().getAllocaAddrSpace(), id.name.c_str(), context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != nullptr) {
		context.dclog << debug_stream::info << "Creating initializer " << type.name << " " << id.name << std::endl;
		NAssignment assn(id, *assignmentExpr);
		context.dclog << debug_stream::indent(2, +1);
		assn.codeGen(context);
		context.dclog << debug_stream::indent(2, -1);
	}
	return alloc;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context)
{
	context.dclog << debug_stream::info << "Creatinge extern declaration " << type.name << " " << id.name << "( ";
	std::vector<Type*> argTypes;
	for (VariableList::const_iterator it = arguments.begin(); it != arguments.end(); ++it) {
		context.dclog << (*it)->type.name << " ";
		argTypes.push_back(typeOf((*it)->type, context));
	}
	context.dclog << ")" << std::endl;
	auto ftype = FunctionType::get(typeOf(type, context), makeArrayRef(argTypes), false);
	auto function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.module);
	return function;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	static std::map<std::string, std::map<std::string, Function*>> def;
	context.dclog << debug_stream::info << "Creating function: " << id.name << std::endl;
	if(def[context.ftrace()].find(id.name) != def[context.ftrace()].end()) {
		context.dclog << "Found function: " << id.name << " at " << context.ftrace() << std::endl;
		if (local) {
			context.locals()[context.ftrace() + "__fn_" + id.name] = def[context.ftrace()][id.name];
		}
		return def[context.ftrace()][id.name];
	}

	std::vector<Type*> argTypes;

	for (auto it = arguments.begin(); it != arguments.end(); ++it) {
		argTypes.push_back(typeOf((*it)->type, context));
	}
	auto ftype = FunctionType::get(typeOf(type, context), makeArrayRef(argTypes), false);
	auto function = local ?
		                Function::Create(ftype, GlobalValue::PrivateLinkage, context.ftrace() + "__fn_" + id.name, context.module)
		                : Function::Create(ftype, GlobalValue::InternalLinkage, id.name, context.module);
	if (local) {
		context.locals()[context.ftrace() + "__fn_" + id.name] = function;
	}
	context.dclog << "Creating basicblock " << function << std::endl;
	auto bblock = BasicBlock::Create(context.llvmContext, "entry", function, nullptr);
	context.pushBlock(bblock, "fn_" + id.name, local, true);
	context.funcBlocks.emplace_back("fn_" + id.name);
	auto argsValues = function->arg_begin();
	std::vector<Value*> storeInst;
	for (auto it = arguments.begin(); it != arguments.end(); ++it) {
		context.dclog << debug_stream::info << "Setting argument " << (*it)->id.name << std::endl;
		context.dclog << debug_stream::indent(2, +1);
		(*it)->codeGen(context);
		Value* argumentValue = &(*argsValues++);
		argumentValue->setName((*it)->id.name);
		auto inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
		storeInst.push_back(inst);
		context.dclog << debug_stream::indent(2, -1);
	}
	context.dclog << "Generating function body for " << id.name << std::endl;
	context.dclog << debug_stream::indent(2, +1);
	auto blockv = block.codeGen(context);
	if (context.getCurrentReturnValue() == nullptr) {
		context.setCurrentReturnValue(blockv);
	}
	context.dclog << debug_stream::indent(2, -1);
	context.dclog << "-Generated function body for " << id.name << std::endl;
	context.dclog << debug_stream::verbose <<  "Current ftrace: " << context.ftrace() << ", " << context.extra[context.ftrace(1) + "__fn_" + id.name].size() << std::endl;
	context.dclog << debug_stream::info;
	if (local && context.extra[context.ftrace(1) + "__fn_" + id.name].size() != 0) {
		context.dclog << "Recreating local function " << id.name << std::endl;
		context.dclog << debug_stream::indent(2, +1);
		context.popBlockUntil(bblock);
		function->eraseFromParent();
		context.popBlock();
		context.funcBlocks.pop_back();
		context.dclog << debug_stream::info;
		context.dclog << debug_stream::indent(2, -1);
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
		for (auto it = arguments.begin(); it != arguments.end(); ++it) {
			context.dclog << debug_stream::info << "Setting argument " << (*it)->id.name << std::endl;
			context.dclog << debug_stream::indent(2, +1);
			(*it)->codeGen(context);
			Value* argumentValue = &(*argsValues++);
			argumentValue->setName((*it)->id.name);
			auto inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
			storeInst.push_back(inst);
			context.dclog << debug_stream::indent(2, -1);
		}
		for (auto ex: context.extra[context.ftrace(1) + "__fn_" + id.name]) {
			context.dclog << debug_stream::info << "Setting extra argument " << ex << std::endl;
			if (argsValues == function->arg_end()) {
				std::cerr << "Argument count mismatch!" << std::endl;
				exit(1);
			}
			Value* argumentPointer = &(*argsValues++);
			argumentPointer->setName(context.ftrace() + ex);
			context.locals()[ex] = argumentPointer;
		}
		context.dclog << "Generating function body for " << id.name << std::endl;
		context.dclog << debug_stream::indent(2, +1);
		blockv = block.codeGen(context);
		if (context.getCurrentReturnValue() == nullptr) {
			context.setCurrentReturnValue(blockv);
		}
		context.dclog << debug_stream::indent(2, -1);
		context.dclog << "-Generated function body for " << id.name << std::endl;
		context.dclog << debug_stream::verbose <<  "Current ftrace: " << context.ftrace(1) << ", " << context.extra[context.ftrace(1) + "__fn_" + id.name].size() << std::endl;
		context.dclog << debug_stream::info;
	}
	
	ReturnInst::Create(context.llvmContext, context.getCurrentReturnValue(), context.currentBlock());
	context.popBlockUntil(bblock);
	context.popBlock();
	context.funcBlocks.pop_back();
	def[context.ftrace()].insert_or_assign(id.name, function);
	return function;
}
