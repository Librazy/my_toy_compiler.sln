#pragma once
#include <llvm/IR/Value.h>
#include <llvm/IR/Constants.h>
#include "ltype.hpp"
namespace LIL {
	typedef llvm::Value* LLVMValpT;
	typedef std::vector<llvm::Value*> LLVMValpsT;
	using LIL::LType;
	class LValue
	{
	public:
		LTycpT type;
		LLVMValpT LLVMVal;
		LValue();
		LValue(LTycpT type, qualifier qualifiers = NONE);
		LValue(LTycpT type, LLVMValpT LLVMVal);
		LValue(LTycpT type, LLVMValpT LLVMVal, qualifier qualifiers);
		qualifier getQualifiers() const;
		bool hasQualifier(qualifier qual) const;
		void setQualifier(qualifier qual);
		void addQualifier(qualifier qual);
	private:
		qualifier qu;
	};
	struct LIntVal : public LValue
	{
		LIntVal(CodeGenC& context, int8_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, int16_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, int32_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, int64_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, uint8_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, uint16_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, uint32_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, uint64_t val, qualifier qualifiers = NONE);

		LIntVal(CodeGenC& context, llvm::StringRef val, uint16_t bitwidth, bool is_signed = false, qualifier qualifiers = NONE);
	};
	struct LFPVal :public LValue
	{
		LFPVal(CodeGenC& context, llvm::StringRef val, FPBitwidth bitwidth = f64, qualifier qualifiers = NONE);

		LFPVal(CodeGenC& context, float_t val, qualifier qualifiers = NONE);

		LFPVal(CodeGenC& context, double_t val, qualifier qualifiers = NONE);
	};
	struct LCharVal : public LValue
	{
		LCharVal(CodeGenC& context, llvm::StringRef val, charBitwidth bitwidth = u8, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, int64_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, int32_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, int16_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, int8_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, uint64_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, uint32_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, uint16_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, uint8_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, wchar_t val, qualifier qualifiers = NONE);

		LCharVal(CodeGenC& context, char val, qualifier qualifiers = NONE);
	};
	struct LTypeVal : public LValue
	{
		LTypeVal(CodeGenC& context, LTycpT ty, qualifier qualifiers = NONE);
	};
}
