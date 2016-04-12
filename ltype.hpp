#pragma once
#include <llvm/IR/DerivedTypes.h>
class LType;
typedef llvm::Type* LLVMTypT;
typedef std::vector<llvm::Type*> LLVMTypsT;
typedef std::vector<LType> LTysT;

class LType
{
public:
	enum Kind
	{
		GENERICS,
		TYPE,
		INT,
		FP,
		CHAR,
		FUNTION,
		ARRAY,
		REF,
		RAWPOINTER
	};

	Kind kind;
	LLVMTypT LLVMTy;
	LTysT dep;
	LType(Kind kind, LTysT dep = LTysT());
	LType(Kind kind, LLVMTypT LLVMTy, LTysT dep = LTysT());
	LType(Kind kind, LLVMTypT LLVMTy, std::initializer_list<LType> deps);

};
class LGenTy:public LType
{
public:
	LGenTy(llvm::LLVMContext& context, LTysT dep);
	LGenTy(llvm::LLVMContext& context, std::initializer_list<LType> dep);
};
class LTypeTy :public LType
{
public:
	LTypeTy(llvm::LLVMContext& context, LType dep);
};
class LIntTy :public LType
{
public:
	uint16_t bitwidth;
	bool is_signed;
	LIntTy(llvm::LLVMContext& context, uint16_t bitwidth, bool is_signed);

	static LIntTy getInt1Ty(llvm::LLVMContext& context);

	static LIntTy getInt8Ty(llvm::LLVMContext& context);

	static LIntTy getInt16Ty(llvm::LLVMContext& context);

	static LIntTy getInt32Ty(llvm::LLVMContext& context);

	static LIntTy getInt64Ty(llvm::LLVMContext& context);

	static LIntTy getInt128Ty(llvm::LLVMContext& context);

	static LIntTy getUInt8Ty(llvm::LLVMContext& context);

	static LIntTy getUInt16Ty(llvm::LLVMContext& context);

	static LIntTy getUInt32Ty(llvm::LLVMContext& context);

	static LIntTy getUInt64Ty(llvm::LLVMContext& context);

	static LIntTy getUInt128Ty(llvm::LLVMContext& context);
};

class LFPTy :public LType
{
public:
	enum FPBitwidth
	{
		f16,
		f32,
		f64,
		f128
	};

	FPBitwidth bitwidth;
	LFPTy(llvm::LLVMContext& context, FPBitwidth bitwidth);

	static LFPTy getF16Ty(llvm::LLVMContext& context);

	static LFPTy getF32Ty(llvm::LLVMContext& context);

	static LFPTy getF64Ty(llvm::LLVMContext& context);

	static LFPTy getF128Ty(llvm::LLVMContext& context);
};

class LCharTy :public LType
{
public:
	enum charBitwidth
	{
		u8,
		s8,
		u16,
		s16,
		u32,
		s32,
		u64,
		s64
	};

	charBitwidth bitwidth;
	bool is_signed;
	LCharTy(llvm::LLVMContext& context, charBitwidth bitwidth);

	static LCharTy getChar8(llvm::LLVMContext& context);

	static LCharTy getChar16(llvm::LLVMContext& context);

	static LCharTy getChar32(llvm::LLVMContext& context);

	static LCharTy getChar64(llvm::LLVMContext& context);

	static LCharTy getUChar8(llvm::LLVMContext& context);

	static LCharTy getUChar16(llvm::LLVMContext& context);

	static LCharTy getUChar32(llvm::LLVMContext& context);

	static LCharTy getUChar64(llvm::LLVMContext& context);
};

class LFunctionTy :public LType
{
public:
	bool is_var_arg;
	LFunctionTy(llvm::LLVMContext& context, LTysT arg_and_ret, bool is_var_arg);
	LFunctionTy(llvm::LLVMContext& context, LTysT arg, LType ret, bool is_var_arg);
};

class LArrayTy :public LType
{
public:
	uint64_t size;
	LArrayTy(llvm::LLVMContext& context, LType type, uint64_t size);
};

class LRefTy :public LType
{
public:
	LRefTy(llvm::LLVMContext& context, LType type);
};

class LPointerTy :public LType
{
public:
	LPointerTy(llvm::LLVMContext& context, LType type);
};

