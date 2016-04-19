#pragma once
#include <vector>
#include <llvm/IR/DerivedTypes.h>
#include <set>
namespace LIL {
	class LGenTy;
	class LTypeTy;
	class LIntTy;
	class LFPTy;
	class LCharTy;
	class LFunctionTy;
	class LArrayTy;
	class LRefTy;
	class LPointerTy;
	class LStructTy;
	class LType;
	typedef llvm::Type* LLVMTypT;
	typedef std::vector<llvm::Type*> LLVMTypvT;
	typedef std::vector<LType> LTyvT;
	typedef std::vector<LType*> LTypvT;
	typedef std::vector<LType const*> LTycpvT;
	typedef std::set<LType*> LTypsT;
	typedef std::set<LType const*> LTycpsT;
	typedef LType* LTypT;
	typedef LType const* LTycpT;

	enum FPBitwidth
	{
		f16 = 2,
		f32 = 4,
		f64 = 8,
		f80 = 10,
		f128 = 16
	};
	enum charBitwidth
	{
		s8 = -1,
		u8 = 0,
		s16 = 1,
		u16 = 2,
		s32 = 7,
		u32 = 8,
		s64 = 15,
		u64 = 16
	};
	enum accessSpecifier
	{
		pub,
		pri,
		pro
	};

	enum qualifier
	{
		NONE = 0,

		CONST = 1,

		TEMP = 2,
		CT = 3,// CONST|TEMP,

		UNIQUE = 4,
		CU = 5,// CONST|UNIQUE,
		TU = 6,// TEMP|UNIQUE,
		CTU = 7,// CONST|TEMP|UNIQUE,

		VOLATILE = 8,
		CV = 9,// CONST|VOLATILE,
		TV = 10,// TEMP|VOLATILE,
		CTV = 11,// CONST|TEMP|VOLATILE,
		UV = 12,// UNIQUE|VOLATILE
		CUV = 13,// CONST|UNIQUE|VOLATILE
		TUV = 14,// TEMP|UNIQUE|VOLATILE
		CTUV = 14,// CONST|TEMP|UNIQUE|VOLATILE
	};

	inline qualifier& operator|=(qualifier& q1, qualifier& q2) {
		return q1 = static_cast<qualifier>(static_cast<int>(q1) | static_cast<int>(q2));
	}

}