#pragma once
#include <map>
#include <llvm/IR/Constant.h>
#include "common.hpp"

namespace LIL {


	class CodeGenC
	{
	public:
		llvm::LLVMContext& LLVMC;
		std::map<LTycpsT, LGenTy const*> GenTys;
		std::map<LTycpT, const LTypeTy const*> TypeTys;
		std::map<std::pair<uint16_t, bool>, LIntTy const*> IntTys;
		std::map<LIL::FPBitwidth, LFPTy const*> FPTys;
		std::map<LIL::charBitwidth, LCharTy const*> CharTys;
		std::map<std::tuple<LTycpvT, LTycpT, bool>, LFunctionTy const*> FunctionTys;
		std::map<std::pair<LTycpT, uint64_t>, LArrayTy const*> ArrayTys;
		std::map<LTycpT, LRefTy const*> RefTys;
		std::map<LTycpT, LPointerTy const*> PointerTys;
		std::map<std::string, LStructTy const*> StructTys;
		std::map<LTycpvT, LStructTy const*> LiteralStructTys;
		explicit CodeGenC(llvm::LLVMContext& llvm_context) :LLVMC(llvm_context) {};
		CodeGenC(CodeGenC&) = delete;
		CodeGenC& operator=(CodeGenC&) = delete;
	};
}