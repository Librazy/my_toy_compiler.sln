#pragma once
#include <map>
#include <llvm/IR/Constant.h>
#include "common.hpp"

namespace LIL {


	class CodeGenC
	{
	public:
		llvm::LLVMContext& LLVMC;
		std::map<LTycpsT, const LGenTy const*> GenTys;
		std::map<LTycpT, const LTypeTy const*> TypeTys;
		std::map<std::pair<uint16_t, bool>, const LIntTy const*> IntTys;
		std::map<LIL::FPBitwidth, const LFPTy const*> FPTys;
		std::map<LIL::charBitwidth, const LCharTy const*> CharTys;
		std::map<std::tuple<LTycpvT, LTycpT, bool>, const LFunctionTy const*> FunctionTys;
		std::map<std::pair<LTycpT, uint64_t>, const LArrayTy const*> ArrayTys;
		std::map<LTycpT, const LRefTy const*> RefTys;
		std::map<LTycpT, const LPointerTy const*> PointerTys;
		std::map<std::string, const LStructTy const*> StructTys;
		std::map<LTycpvT, const LStructTy const*> LiteralStructTys;
		explicit CodeGenC(llvm::LLVMContext& llvm_context) :LLVMC(llvm_context) {};
		CodeGenC(CodeGenC&) = delete;
		CodeGenC& operator=(CodeGenC&) = delete;
	};
}
