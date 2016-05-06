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

	LIntVal::LIntVal(CodeGenC& context, int8_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getInt8Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(8, val, true));
	}

	LIntVal::LIntVal(CodeGenC& context, int16_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getInt16Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(16, val, true));
	}

	LIntVal::LIntVal(CodeGenC& context, int32_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getInt32Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(32, val, true));
	}

	LIntVal::LIntVal(CodeGenC& context, int64_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getInt64Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(64, val, true));
	}

	LIntVal::LIntVal(CodeGenC& context, uint8_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getUInt8Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(8, val));
	}

	LIntVal::LIntVal(CodeGenC& context, uint16_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getUInt16Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(16, val));
	}

	LIntVal::LIntVal(CodeGenC& context, uint32_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getUInt32Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(32, val));
	}

	LIntVal::LIntVal(CodeGenC& context, uint64_t val, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getUInt64Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(64, val));
	}

	LIntVal::LIntVal(CodeGenC& context, llvm::StringRef val, uint16_t bitwidth, bool is_signed, qualifier qualifiers /*=NONE*/):
		LValue(LIntTy::getIntTy(context, bitwidth, is_signed), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(bitwidth, val, is_signed));
	}

	LFPVal::LFPVal(CodeGenC& context, llvm::StringRef val, FPBitwidth bitwidth, qualifier qualifiers /*=NONE*/):
		LValue(LFPTy::getFPTy(context, bitwidth), qualifiers)
	{
		LLVMVal = llvm::ConstantFP::get(type->LLVMTy, val);
	}

	LFPVal::LFPVal(CodeGenC& context, float_t val, qualifier qualifiers /*=NONE*/):
		LValue(LFPTy::getF32Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantFP::get(type->LLVMTy, val);
	}

	LFPVal::LFPVal(CodeGenC& context, double_t val, qualifier qualifiers /*=NONE*/):
		LValue(LFPTy::getF64Ty(context), qualifiers)
	{
		LLVMVal = llvm::ConstantFP::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, llvm::StringRef val, charBitwidth bitwidth, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, bitwidth), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(8, val, false));
	}

	LCharVal::LCharVal(CodeGenC& context, int64_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, s64), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, int32_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, s32), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, int16_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, s16), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, int8_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, s8), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, uint64_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, u64), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, uint32_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, u32), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, uint16_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, u16), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, uint8_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context, u8), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	// ReSharper disable CppUnreachableCode
	LCharVal::LCharVal(CodeGenC& context, wchar_t val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context,
		                          std::is_signed<wchar_t>::value ? (
			                          std::numeric_limits<wchar_t>::max() > 0x7fff ?
				                          s32 :
				                          s16
		                          ) : (
			                          std::numeric_limits<wchar_t>::max() > 0xffff ?
				                          u32 :
				                          u16
		                          )
		                         ), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}

	LCharVal::LCharVal(CodeGenC& context, char val, qualifier qualifiers /*=NONE*/):
		LValue(LCharTy::getCharTy(context,
		                          std::is_signed<char>::value ?
			                          s8 :
			                          u8

		                         ), qualifiers)
	{
		LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
	}
	// ReSharper restore CppUnreachableCode

	LTypeVal::LTypeVal(CodeGenC& context, LTycpT ty, qualifier qualifiers):
		LValue(LTypeTy::getTypeTy(context, ty), qualifiers)
	{

	}

}
