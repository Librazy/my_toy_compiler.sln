#include "node.hpp"

llvm::Value * NBool::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NInteger::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NDouble::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NIdentifier::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NMethodCall::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NBinaryOperator::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NAssignment::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NBlock::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NReturn::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NIfBlock::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NWhileBlock::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NVariableDefinition::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NExternDeclaration::codeGen(CodeGenContext & context)
{
	return nullptr;
}

llvm::Value * NFunctionDeclaration::codeGen(CodeGenContext & context)
{
	return nullptr;
}
