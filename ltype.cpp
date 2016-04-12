#include"ltype.hpp"

LType::LType(Kind kind, LTysT dep /*= LTysT()*/) : kind(kind), LLVMTy(nullptr), dep(dep) {}

LType::LType(Kind kind, LLVMTypT LLVMTy, LTysT dep /*= LTysT()*/) : kind(kind), LLVMTy(LLVMTy), dep(dep) {}

LType::LType(Kind kind, LLVMTypT LLVMTy, std::initializer_list<LType> deps) : kind(kind), LLVMTy(LLVMTy)
{
	for (auto i : deps) {
		dep.emplace_back(i);
	}
}

LGenTy::LGenTy(llvm::LLVMContext& context, LTysT dep) : LType(GENERICS, llvm::Type::getVoidTy(context), dep) {}

LGenTy::LGenTy(llvm::LLVMContext& context, std::initializer_list<LType> dep) : LType(GENERICS, llvm::Type::getVoidTy(context), dep) {}

LTypeTy::LTypeTy(llvm::LLVMContext& context, LType dep) : LType(TYPE, llvm::Type::getVoidTy(context), { dep }) {}

LIntTy::LIntTy(llvm::LLVMContext& context, uint16_t bitwidth, bool is_signed) :
	LType(INT, llvm::Type::getIntNTy(context, bitwidth))
	, bitwidth(bitwidth), is_signed(is_signed) {}

LIntTy LIntTy::getInt1Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 1, false);
}

LIntTy LIntTy::getInt8Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 8, true);
}

LIntTy LIntTy::getInt16Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 16, true);
}

LIntTy LIntTy::getInt32Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 32, true);
}

LIntTy LIntTy::getInt64Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 64, true);
}

LIntTy LIntTy::getInt128Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 128, true);
}

LIntTy LIntTy::getUInt8Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 8, false);
}

LIntTy LIntTy::getUInt16Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 16, false);
}

LIntTy LIntTy::getUInt32Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 32, false);
}

LIntTy LIntTy::getUInt64Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 64, false);
}

LIntTy LIntTy::getUInt128Ty(llvm::LLVMContext& context)
{
	return LIntTy(context, 128, false);
}

LFPTy::LFPTy(llvm::LLVMContext& context, FPBitwidth bitwidth) : LType(FP), bitwidth(bitwidth)
{
	switch (bitwidth) {
	case f16:
		LLVMTy = llvm::Type::getHalfTy(context);
		break;
	case f32:
		LLVMTy = llvm::Type::getFloatTy(context);
		break;
	case f64:
		LLVMTy = llvm::Type::getDoubleTy(context);
		break;
	case f128:
		LLVMTy = llvm::Type::getFP128Ty(context);
		break;
	}
}

LFPTy LFPTy::getF16Ty(llvm::LLVMContext& context)
{
	return LFPTy(context, f16);
}

LFPTy LFPTy::getF32Ty(llvm::LLVMContext& context)
{
	return LFPTy(context, f32);
}

LFPTy LFPTy::getF64Ty(llvm::LLVMContext& context)
{
	return LFPTy(context, f64);
}

LFPTy LFPTy::getF128Ty(llvm::LLVMContext& context)
{
	return LFPTy(context, f128);
}

LCharTy::LCharTy(llvm::LLVMContext& context, charBitwidth bitwidth) : LType(CHAR), bitwidth(bitwidth)
{
	switch (bitwidth) {
	case u8:
		LLVMTy = llvm::Type::getIntNTy(context, 8);
		is_signed = false;
		break;
	case s8:
		LLVMTy = llvm::Type::getIntNTy(context, 8);
		is_signed = true;
		break;
	case u16:
		LLVMTy = llvm::Type::getIntNTy(context, 16);
		is_signed = false;
		break;
	case s16:
		LLVMTy = llvm::Type::getIntNTy(context, 16);
		is_signed = true;
		break;
	case u32:
		LLVMTy = llvm::Type::getIntNTy(context, 32);
		is_signed = false;
		break;
	case s32:
		LLVMTy = llvm::Type::getIntNTy(context, 32);
		is_signed = true;
		break;
	case u64:
		LLVMTy = llvm::Type::getIntNTy(context, 32);
		is_signed = false;
		break;
	case s64:
		LLVMTy = llvm::Type::getIntNTy(context, 32);
		is_signed = true;
		break;
	}
}

LCharTy LCharTy::getChar8(llvm::LLVMContext& context)
{
	return LCharTy(context, s8);
}

LCharTy LCharTy::getChar16(llvm::LLVMContext& context)
{
	return LCharTy(context, s16);
}

LCharTy LCharTy::getChar32(llvm::LLVMContext& context)
{
	return LCharTy(context, s32);
}

LCharTy LCharTy::getChar64(llvm::LLVMContext& context)
{
	return LCharTy(context, s64);
}

LCharTy LCharTy::getUChar8(llvm::LLVMContext& context)
{
	return LCharTy(context, u8);
}

LCharTy LCharTy::getUChar16(llvm::LLVMContext& context)
{
	return LCharTy(context, u16);
}

LCharTy LCharTy::getUChar32(llvm::LLVMContext& context)
{
	return LCharTy(context, u32);
}

LCharTy LCharTy::getUChar64(llvm::LLVMContext& context)
{
	return LCharTy(context, u64);
}

LFunctionTy::LFunctionTy(llvm::LLVMContext& context, LTysT arg_and_ret, bool is_var_arg) :
	LType(FUNTION, arg_and_ret), is_var_arg(is_var_arg)
{
	LType ret = arg_and_ret.back();
	arg_and_ret.pop_back();
	LLVMTypsT llarg;
	std::transform(arg_and_ret.begin(), arg_and_ret.end(), std::back_inserter(llarg), [](LType i) {return i.LLVMTy; });
	LLVMTy = llvm::FunctionType::get(ret.LLVMTy, llvm::ArrayRef<LLVMTypT>(llarg), is_var_arg);
}

LFunctionTy::LFunctionTy(llvm::LLVMContext& context, LTysT arg, LType ret, bool is_var_arg) :
	LType(FUNTION, arg), is_var_arg(is_var_arg)
{
	dep.push_back(ret);
	LLVMTypsT llarg;
	std::transform(arg.begin(), arg.end(), std::back_inserter(llarg), [](LType i) {return i.LLVMTy; });
	LLVMTy = llvm::FunctionType::get(ret.LLVMTy, llvm::ArrayRef<LLVMTypT>(llarg), is_var_arg);
}

LArrayTy::LArrayTy(llvm::LLVMContext& context, LType type, uint64_t size) :
	LType(ARRAY, llvm::ArrayType::get(type.LLVMTy, size), { type }), size(size) {}

LRefTy::LRefTy(llvm::LLVMContext& context, LType type) :
	LType(REF, llvm::PointerType::get(type.LLVMTy, 0), { type }) {}

LPointerTy::LPointerTy(llvm::LLVMContext& context, LType type) :
	LType(RAWPOINTER, llvm::PointerType::get(type.LLVMTy, 0), { type }) {}