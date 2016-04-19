#include <llvm/IR/Value.h>
#include "ltype.hpp"
#include "lvalue.hpp"
namespace LIL {
	LValue::LValue() :type(), LLVMVal(nullptr), qu(NONE) {}

	LValue::LValue(LTycpT type, qualifier qualifiers /*=NONE*/) : type(type), LLVMVal(nullptr), qu(qualifiers) {}

	LValue::LValue(LTycpT type, LLVMValpT LLVMVal) : type(type), LLVMVal(LLVMVal), qu(NONE) {}

	LValue::LValue(LTycpT type, LLVMValpT LLVMVal, qualifier qualifiers) : type(type), LLVMVal(LLVMVal), qu(qualifiers) {}

	qualifier LValue::getQualifiers() const
	{
		return qu;
	}

	bool LValue::hasQualifier(qualifier qual) const
	{
		return qual | qu;
	}

	void LValue::setQualifier(qualifier qual)
	{
		qu = qual;
	}

	void LValue::addQualifier(qualifier qual)
	{
		qu |= qual;
	}
}
