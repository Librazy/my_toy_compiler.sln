#include "codegencontext.hpp"
#include "ltype.hpp"
namespace LIL {

	LType::LType() : kind(VOID), LLVMTy(nullptr), dep() {}

	std::string LType::getID() const{return "";}

	LType::LType(Kind kind, LTycpvT dep /*= LTycpvT()*/) : kind(kind), LLVMTy(nullptr), dep(dep) {}

	LType::LType(Kind kind, LLVMTypT LLVMTy, LTycpvT dep /*= LTycpvT()*/) : kind(kind), LLVMTy(LLVMTy), dep(dep) {}

	LType::LType(Kind kind, LLVMTypT LLVMTy, std::initializer_list<LTycpT> deps) : kind(kind), LLVMTy(LLVMTy)
	{
		for (auto i : deps) { dep.emplace_back(i); }
	}


	LGenTy::LGenTy(CodeGenC& context, LTycpvT dep) : LType(GENERICS, llvm::Type::getVoidTy(context.LLVMC), dep) {}

	LGenTy::LGenTy(CodeGenC& context, std::initializer_list<LTycpT> dep) : LType(GENERICS, llvm::Type::getVoidTy(context.LLVMC), dep) {}

	LGenTy::LGenTy(CodeGenC& context, LTycpsT dep) : LType(GENERICS, llvm::Type::getVoidTy(context.LLVMC), LTycpvT(dep.begin(), dep.end())) {}

	std::string LGenTy::getID() const
	{
		std::string ret = "(";
		for (auto i : dep) {
			ret += i->getID() + "&";
		}
		ret.back() = ')';
		return ret;
	}

	LGenTy const* LGenTy::getGenTy(CodeGenC& context, LTycpsT dep)
	{
		auto i = context.GenTys.find(dep);
		if (i == context.GenTys.end()) {
			auto r = context.GenTys.insert(std::make_pair(dep, new LGenTy(context, dep)));
			return r.first->second;
		}
		return i->second;
	}

	LGenTy const* LGenTy::getGenTy(CodeGenC& context, LTycpvT dep)
	{
		LTycpsT set(dep.begin(), dep.end());
		auto i = context.GenTys.find(set);
		if (i == context.GenTys.end()) {
			auto r = context.GenTys.insert(std::make_pair(set, new LGenTy(context, set)));
			return r.first->second;
		}
		return i->second;
	}

	LGenTy const* LGenTy::getGenTy(CodeGenC& context, std::initializer_list<LTycpT> dep)
	{
		return getGenTy(context, LTycpvT(dep));
	}

	LTypeTy::LTypeTy(CodeGenC& context, LTycpT dep) : LType(TYPE, llvm::Type::getVoidTy(context.LLVMC), { dep }) {}

	std::string LTypeTy::getID() const
	{
		return "Ty("+(*dep.begin())->getID()+")";
	}

	LTypeTy const* LTypeTy::getTypeTy(CodeGenC& context, LTycpT dep)
	{
		auto i = context.TypeTys.find(dep);
		if (i == context.TypeTys.end()) {
			auto r = context.TypeTys.insert(std::make_pair(dep, new LTypeTy(context, dep)));
			return r.first->second;
		}
		return i->second;
	}

	LIntTy::LIntTy(CodeGenC& context, uint16_t bitwidth, bool is_signed) :
		LType(INT, llvm::Type::getIntNTy(context.LLVMC, bitwidth))
		, bitwidth(bitwidth), is_signed(is_signed) {}

	std::string LIntTy::getID() const
	{
		return is_signed?"i":"u"+bitwidth;
	}

	LIntTy const* LIntTy::getIntTy(CodeGenC& context, uint16_t bitwidth, bool is_signed)
	{
		auto key = std::make_pair(bitwidth, is_signed);
		auto i = context.IntTys.find(key);
		if (i == context.IntTys.end()) {
			auto ret = context.IntTys.insert(std::make_pair(key, new LIntTy(context, bitwidth, is_signed)));
			return ret.first->second;
		}
		return i->second;
	}

	LIntTy const* LIntTy::getInt1Ty(CodeGenC& context)
	{
		return getIntTy(context, 1, false);
	}
	LIntTy const* LIntTy::getInt8Ty(CodeGenC& context)
	{
		return getIntTy(context, 8, true);
	}
	LIntTy const* LIntTy::getInt16Ty(CodeGenC& context)
	{
		return getIntTy(context, 16, true);
	}
	LIntTy const* LIntTy::getInt32Ty(CodeGenC& context)
	{
		return getIntTy(context, 32, true);
	}
	LIntTy const* LIntTy::getInt64Ty(CodeGenC& context)
	{
		return getIntTy(context, 64, true);
	}
	LIntTy const* LIntTy::getInt128Ty(CodeGenC& context)
	{
		return getIntTy(context, 128, true);
	}
	LIntTy const* LIntTy::getUInt8Ty(CodeGenC& context)
	{
		return getIntTy(context, 8, false);
	}
	LIntTy const* LIntTy::getUInt16Ty(CodeGenC& context)
	{
		return getIntTy(context, 16, false);
	}
	LIntTy const* LIntTy::getUInt32Ty(CodeGenC& context)
	{
		return getIntTy(context, 32, false);
	}
	LIntTy const* LIntTy::getUInt64Ty(CodeGenC& context)
	{
		return getIntTy(context, 64, false);
	}
	LIntTy const* LIntTy::getUInt128Ty(CodeGenC& context)
	{
		return getIntTy(context, 128, false);
	}

	bool LIntTy::canContain(const LIntTy const* source) const
	{
		if (this == source)
			return true;
		if (!this->is_signed&&source->is_signed)
			return false;
		if (this->bitwidth - static_cast<int>(this->is_signed) >= source->bitwidth - static_cast<int>(source->is_signed))
			return true;

		return false;
	}

	LFPTy::LFPTy(CodeGenC& context, LIL::FPBitwidth bitwidth) : LType(FP), bitwidth(bitwidth)
	{
		switch (bitwidth) {
		case LIL::f16:
			LLVMTy = llvm::Type::getHalfTy(context.LLVMC);
			break;
		case LIL::f32:
			LLVMTy = llvm::Type::getFloatTy(context.LLVMC);
			break;
		case LIL::f64:
			LLVMTy = llvm::Type::getDoubleTy(context.LLVMC);
			break;
		case LIL::f80:
			LLVMTy = llvm::Type::getX86_FP80Ty(context.LLVMC);
			break;
		case LIL::f128:
			LLVMTy = llvm::Type::getFP128Ty(context.LLVMC);
			break;
		}
	}

	std::string LFPTy::getID() const
	{
		switch (bitwidth) {
		case LIL::f16:
			return "f16";
		case LIL::f32:
			return "f32";
		case LIL::f64:
			return "f64";
		case LIL::f80:
			return "f80";
		case LIL::f128:
			return "f128";
		}
		return "";
	}

	LFPTy const* LFPTy::getFPTy(CodeGenC& context, LIL::FPBitwidth bitwidth)
	{
		auto i = context.FPTys.find(bitwidth);
		if (i == context.FPTys.end()) {
			auto ret = context.FPTys.insert(std::make_pair(bitwidth, new LFPTy(context, bitwidth)));
			return ret.first->second;
		}
		return i->second;
	}

	LFPTy const* LFPTy::getF16Ty(CodeGenC& context)
	{
		return getFPTy(context, LIL::f16);
	}
	LFPTy const* LFPTy::getF32Ty(CodeGenC& context)
	{
		return getFPTy(context, LIL::f32);
	}
	LFPTy const* LFPTy::getF64Ty(CodeGenC& context)
	{
		return getFPTy(context, LIL::f64);
	}
	LFPTy const* LFPTy::getF80Ty(CodeGenC& context)
	{
		return getFPTy(context, LIL::f80);
	}
	LFPTy const* LFPTy::getF128Ty(CodeGenC& context)
	{
		return getFPTy(context, LIL::f128);
	}

	bool LFPTy::canContain(const LFPTy const * source) const
	{
		if (this == source)
			return true;
		if(bitwidth >= source->bitwidth)return true;
		return false;
	}

	LCharTy::LCharTy(CodeGenC& context, LIL::charBitwidth bitwidth) : LType(CHAR), bitwidth(bitwidth)
	{
		switch (bitwidth) {
		case LIL::u8:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 8);
			is_signed = false;
			break;
		case LIL::s8:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 8);
			is_signed = true;
			break;
		case LIL::u16:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 16);
			is_signed = false;
			break;
		case LIL::s16:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 16);
			is_signed = true;
			break;
		case LIL::u32:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 32);
			is_signed = false;
			break;
		case LIL::s32:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 32);
			is_signed = true;
			break;
		case LIL::u64:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 32);
			is_signed = false;
			break;
		case LIL::s64:
			LLVMTy = llvm::Type::getIntNTy(context.LLVMC, 32);
			is_signed = true;
			break;
		}
	}

	std::string LCharTy::getID() const
	{
		switch (bitwidth) {
		case LIL::u8:
			return "u8";
		case LIL::s8:
			return "s8";
		case LIL::u16:
			return "u16";
		case LIL::s16:
			return "s16";
		case LIL::u32:
			return "u32";
		case LIL::s32:
			return "s32";
		case LIL::u64:
			return "s64";
		case LIL::s64:
			return "s64";
		}
		return "";
	}

	LCharTy const* LCharTy::getCharTy(CodeGenC& context, LIL::charBitwidth bitwidth)
	{
		auto i = context.CharTys.find(bitwidth);
		if (i == context.CharTys.end()) {
			auto ret = context.CharTys.insert(std::make_pair(bitwidth, new LCharTy(context, bitwidth)));
			return ret.first->second;
		}
		return i->second;
	}

	LCharTy const* LCharTy::getChar8Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::s8);
	}
	LCharTy const* LCharTy::getChar16Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::s16);
	}
	LCharTy const* LCharTy::getChar32Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::s16);
	}
	LCharTy const* LCharTy::getChar64Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::s64);
	}
	LCharTy const* LCharTy::getUChar8Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::u8);
	}
	LCharTy const* LCharTy::getUChar16Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::u16);
	}
	LCharTy const* LCharTy::getUChar32Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::u32);
	}
	LCharTy const* LCharTy::getUChar64Ty(CodeGenC& context)
	{
		return getCharTy(context, LIL::u64);
	}

	bool LCharTy::canContain(const LCharTy* source) const
	{
		if (this == source)
			return true;
		if (bitwidth >= source->bitwidth)return true;
		return false;
	}

	LFunctionTy::LFunctionTy(CodeGenC& context, LTycpvT arg_and_ret, bool is_var_arg) :
		LType(FUNTION, arg_and_ret), is_var_arg(is_var_arg)
	{
		LTycpT ret = arg_and_ret.back();
		arg_and_ret.pop_back();
		LLVMTypvT llarg;
		std::transform(arg_and_ret.begin(), arg_and_ret.end(), std::back_inserter(llarg), [](LTycpT i) {return i->LLVMTy; });
		LLVMTy = llvm::FunctionType::get(ret->LLVMTy, llvm::ArrayRef<LLVMTypT>(llarg), is_var_arg);
	}

	LFunctionTy::LFunctionTy(CodeGenC& context, LTycpvT arg, LTycpT ret, bool is_var_arg) :
		LType(FUNTION, arg), is_var_arg(is_var_arg)
	{
		dep.push_back(ret);
		LLVMTypvT llarg;
		std::transform(arg.begin(), arg.end(), std::back_inserter(llarg), [](LTycpT i) {return i->LLVMTy; });
		LLVMTy = llvm::FunctionType::get(ret->LLVMTy, llvm::ArrayRef<LLVMTypT>(llarg), is_var_arg);
	}

	std::string LFunctionTy::getID() const
	{
		std::string ret = "( ";
		std::for_each(dep.begin(), --dep.end(), [&ret](LTycpT i) {ret += i->getID() + " -> "; });
		ret += (*--dep.end())->getID() + " )";
		return ret;
	}

	LFunctionTy const* LFunctionTy::getFunctionTy(CodeGenC& context, LTycpvT arg_and_ret, bool is_var_arg)
	{
		auto ret = arg_and_ret.back();
		arg_and_ret.pop_back();
		auto key = std::make_tuple(arg_and_ret, ret, is_var_arg);

		auto i = context.FunctionTys.find(key);
		if (i == context.FunctionTys.end()) {
			auto r = context.FunctionTys.insert(std::make_pair(key, new LFunctionTy(context, arg_and_ret, ret)));
			return r.first->second;
		}
		return i->second;
	}

	LFunctionTy const* LFunctionTy::getFunctionTy(CodeGenC& context, LTycpvT arg, LTycpT ret, bool is_var_arg)
	{
		auto key = std::make_tuple(arg, ret, is_var_arg);

		auto i = context.FunctionTys.find(key);
		if (i == context.FunctionTys.end()) {
			auto r = context.FunctionTys.insert(std::make_pair(key, new LFunctionTy(context, arg, ret)));
			return r.first->second;
		}
		return i->second;

	}

	LArrayTy::LArrayTy(CodeGenC& context, LTycpT type, uint64_t size) :
		LType(ARRAY, llvm::ArrayType::get(type->LLVMTy, size), { type }), size(size) {}

	std::string LArrayTy::getID() const
	{
		return dep.front()->getID() + "[" + std::to_string(size) + "]";
	}

	LArrayTy const* LArrayTy::getArrayTy(CodeGenC& context, LTycpT type, uint64_t size)
	{
		auto key = std::make_pair(type, size);

		auto i = context.ArrayTys.find(key);
		if (i == context.ArrayTys.end()) {
			auto r = context.ArrayTys.insert(std::make_pair(key, new LArrayTy(context, type, size)));
			return r.first->second;
		}
		return i->second;
	}

	LRefTy::LRefTy(CodeGenC& context, LTycpT type) :
		LType(REF, llvm::PointerType::get(type->LLVMTy, 0), { type }) {}

	std::string LRefTy::getID() const
	{
		return dep.front()->getID() + "&";
	}

	LRefTy const* LRefTy::getRefTy(CodeGenC& context, LTycpT type)
	{
		auto i = context.RefTys.find(type);
		if (i == context.RefTys.end()) {
			auto r = context.RefTys.insert(std::make_pair(type, new LRefTy(context, type)));
			return r.first->second;
		}
		return i->second;
	}

	LPointerTy::LPointerTy(CodeGenC& context, LTycpT type) :
		LType(RAWPOINTER, llvm::PointerType::get(type->LLVMTy, 0), { type }) {}

	std::string LPointerTy::getID() const
	{
		return dep.front()->getID() + "*";
	}

	LPointerTy const* LPointerTy::getPointerTy(CodeGenC& context, LTycpT type)
	{
		auto i = context.PointerTys.find(type);
		if (i == context.PointerTys.end()) {
			auto r = context.PointerTys.insert(std::make_pair(type, new LPointerTy(context, type)));
			return r.first->second;
		}
		return i->second;
	}

	LStructTy::LStructTy(CodeGenC& context, std::string name, std::vector<std::pair<LTycpT, accessSpecifier>> type) :
		LType(STRUCT),name(name),fields(type)
	{
		auto structTy = llvm::StructType::get(context.LLVMC);
		std::vector<LLVMTypT> fieldTypes;
		std::transform(type.begin(), type.end(), fieldTypes.begin(), 
			[&dep=(this->dep)](std::pair<LTycpT, accessSpecifier> i) {dep.push_back(i.first); return i.first->LLVMTy; });
		structTy->setBody(makeArrayRef(fieldTypes));
		structTy->setName(llvm::StringRef(name));
		LLVMTy = structTy;
	}

	LStructTy::LStructTy(CodeGenC& context, LTycpvT type) :
		LType(STRUCT)
	{
		
		std::vector<LLVMTypT> fieldTypes;
		std::string name="{";
		std::transform(type.begin(), type.end(), fieldTypes.begin(), 
			[&dep = (this->dep), &fields = (this->fields), &name](LTycpT i) {dep.push_back(i); name += i->getID() + ","; fields.emplace_back(std::make_pair(i, pub)); return i->LLVMTy;});
		name.back()='}';
		auto structTy = llvm::StructType::get(context.LLVMC, llvm::makeArrayRef(fieldTypes));
		this->name = name;
		LLVMTy = structTy;
	}

	std::string LStructTy::getID() const
	{
		return name;
	}

	LStructTy const* LStructTy::setStructTy(CodeGenC& context, std::string name, std::vector<std::pair<LTycpT, accessSpecifier>> type)
	{
		auto i = context.StructTys.find(name);
		if (i == context.StructTys.end()) {
			auto r = context.StructTys.insert(std::make_pair(name, new LStructTy(context, name, type)));
			return r.first->second;
		}
		return i->second;
	}

	LStructTy const* LStructTy::getStructTy(CodeGenC& context, std::string name)
	{
		auto i = context.StructTys.find(name);
		if (i == context.StructTys.end()) {
			return nullptr;
		}
		return i->second;
	}

	LStructTy const* LStructTy::getStructTy(CodeGenC& context, LTycpvT type)
	{
		auto i = context.LiteralStructTys.find(type);
		if (i == context.LiteralStructTys.end()) {
			auto r = context.LiteralStructTys.insert(std::make_pair(type, new LStructTy(context, type)));
			return r.first->second;
		}
		return i->second;
	}
}
