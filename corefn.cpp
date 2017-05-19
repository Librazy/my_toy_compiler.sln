#include "codegen.h"
#include "node.h"

using namespace std;

extern int yyparse();
extern NBlock* programBlock;


Function* createPrintfFunction(CodeGenContext& context)
{
	vector<Type*> printf_arg_types;
	printf_arg_types.push_back(Type::getInt8PtrTy(context.llvmContext)); //char*

	FunctionType* printf_type =
			FunctionType::get(
				Type::getInt32Ty(context.llvmContext), makeArrayRef(printf_arg_types), true);

	Function* func = Function::Create(
		printf_type, Function::ExternalLinkage,
		Twine("printf"),
		context.module
	);
	func->setCallingConv(CallingConv::C);
	return func;
}

void createEchoFunction(CodeGenContext& context, Function* printfFn)
{
	vector<Type*> echo_arg_types;
	echo_arg_types.push_back(Type::getInt64Ty(context.llvmContext));

	auto echo_type =
			FunctionType::get(
				Type::getInt64Ty(context.llvmContext), makeArrayRef(echo_arg_types), false);

	auto func = Function::Create(
		echo_type, Function::InternalLinkage,
		Twine("echo"),
		context.module
	);
	auto bblock = BasicBlock::Create(context.llvmContext, "entry", func, nullptr);
	context.pushBlock(bblock, "echo");

	auto constValue = "%d\n";
	auto format_const = ConstantDataArray::getString(context.llvmContext, constValue);
	auto var =
			new GlobalVariable(
				*context.module, ArrayType::get(IntegerType::get(context.llvmContext, 8), strlen(constValue) + 1),
				true, GlobalValue::PrivateLinkage, format_const, ".str");
	auto zero =
			Constant::getNullValue(IntegerType::getInt32Ty(context.llvmContext));

	vector<Constant*> indices;
	indices.push_back(zero);
	indices.push_back(zero);
	auto var_ref = ConstantExpr::getGetElementPtr(
		ArrayType::get(IntegerType::get(context.llvmContext, 8), strlen(constValue) + 1), var, indices);
	vector<Value*> args;
	args.push_back(var_ref);

	auto argsValues = func->arg_begin();
	auto toPrint = &*argsValues;
	toPrint->setName("toPrint");
	args.push_back(toPrint);

	CallInst::Create(printfFn, makeArrayRef(args), "", bblock);
	ReturnInst::Create(context.llvmContext, toPrint, bblock);
	context.popBlock();
}

void createEchodFunction(CodeGenContext& context, Function* printfFn)
{
	vector<Type*> echo_arg_types;
	echo_arg_types.push_back(Type::getDoubleTy(context.llvmContext));

	auto echo_type =
			FunctionType::get(
				Type::getDoubleTy(context.llvmContext), echo_arg_types, false);

	auto func = Function::Create(
		echo_type, Function::InternalLinkage,
		Twine("echod"),
		context.module
	);
	auto bblock = BasicBlock::Create(context.llvmContext, "entry", func, nullptr);
	context.pushBlock(bblock, "echod");

	auto constValue = "%lf\n";
	auto format_const = ConstantDataArray::getString(context.llvmContext, constValue);
	auto var =
			new GlobalVariable(
				*context.module, ArrayType::get(IntegerType::get(context.llvmContext, 8), strlen(constValue) + 1),
				true, GlobalValue::PrivateLinkage, format_const, ".str");
	auto zero =
			Constant::getNullValue(IntegerType::getInt32Ty(context.llvmContext));

	vector<Constant*> indices;
	indices.push_back(zero);
	indices.push_back(zero);
	auto var_ref = ConstantExpr::getGetElementPtr(
		nullptr, var, indices);
	vector<Value*> args;
	args.push_back(var_ref);

	auto argsValues = func->arg_begin();
	auto toPrint = &*argsValues;
	toPrint->setName("toPrint");
	args.push_back(toPrint);
	CallInst::Create(printfFn, makeArrayRef(args), "", bblock);
	ReturnInst::Create(context.llvmContext, toPrint, bblock);
	context.popBlock();
}

void createCoreFunctions(CodeGenContext& context)
{
	Function* printfFn = createPrintfFunction(context);
	createEchoFunction(context, printfFn);
	createEchodFunction(context, printfFn);
}
