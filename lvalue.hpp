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
	class LIntVal : public LValue
	{
		LIntVal(CodeGenC& context, int8_t val) :
			LValue(LIntTy::getInt8Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(8, val, true));
		}
		LIntVal(CodeGenC& context, int16_t val) :
			LValue(LIntTy::getInt16Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(16, val, true));
		}
		LIntVal(CodeGenC& context, int32_t val) :
			LValue(LIntTy::getInt32Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(32, val, true));
		}
		LIntVal(CodeGenC& context, int64_t val) :
			LValue(LIntTy::getInt64Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(64, val, true));
		}
		LIntVal(CodeGenC& context, uint8_t val) :
			LValue(LIntTy::getUInt8Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(8, val));
		}
		LIntVal(CodeGenC& context, uint16_t val) :
			LValue(LIntTy::getUInt16Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(16, val));
		}
		LIntVal(CodeGenC& context, uint32_t val) :
			LValue(LIntTy::getUInt32Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(32, val));
		}
		LIntVal(CodeGenC& context, uint64_t val) :
			LValue(LIntTy::getUInt64Ty(context))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(64, val));
		}
		LIntVal(CodeGenC& context, llvm::StringRef val, uint16_t bitwidth, bool is_signed = false) :
			LValue(LIntTy::getIntTy(context, bitwidth, is_signed))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(bitwidth, val, is_signed));
		}

	};
	class LFPVal :public LValue
	{
		LFPVal(CodeGenC& context, llvm::StringRef val, FPBitwidth bitwidth = f64) :
			LValue(LFPTy::getFPTy(context,bitwidth))
		{
			LLVMVal = llvm::ConstantFP::get(type->LLVMTy, val);
		}
		LFPVal(CodeGenC& context, float_t val) :
			LValue(LFPTy::getF32Ty(context))
		{
			LLVMVal = llvm::ConstantFP::get(type->LLVMTy, val);
		}
		LFPVal(CodeGenC& context, double_t val) :
			LValue(LFPTy::getF64Ty(context))
		{
			LLVMVal = llvm::ConstantFP::get(type->LLVMTy, val);
		}
	};
	class LCharVal : public LValue
	{
		LCharVal(CodeGenC& context, llvm::StringRef val, charBitwidth bitwidth = u8) :
			LValue(LCharTy::getCharTy(context,bitwidth))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, llvm::APInt(8, val, false));
		}
		LCharVal(CodeGenC& context, int64_t val) :
			LValue(LCharTy::getCharTy(context, s64))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, int32_t val) :
			LValue(LCharTy::getCharTy(context, s32))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, int16_t val) :
			LValue(LCharTy::getCharTy(context, s16))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, int8_t val) :
			LValue(LCharTy::getCharTy(context, s8))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, uint64_t val) :
			LValue(LCharTy::getCharTy(context, u64))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, uint32_t val) :
			LValue(LCharTy::getCharTy(context, u32))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, uint16_t val) :
			LValue(LCharTy::getCharTy(context, u16))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, uint8_t val) :
			LValue(LCharTy::getCharTy(context, u8))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		// ReSharper disable CppUnreachableCode
		LCharVal(CodeGenC& context, wchar_t val) :
			LValue(LCharTy::getCharTy(context, 
				std::is_signed<wchar_t>::value ?(
					std::numeric_limits<wchar_t>::max()>0xffff ?
					s32:
					s16
					
				):(
					std::numeric_limits<wchar_t>::max()>0xffff ?
					u32:
					u16
				)
			))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		LCharVal(CodeGenC& context, char val) :
			LValue(LCharTy::getCharTy(context,
				std::is_signed<char>::value ? 
					s8:
					u8
			        
			))
		{
			LLVMVal = llvm::ConstantInt::get(type->LLVMTy, val);
		}
		// ReSharper restore CppUnreachableCode
	};
}
