#pragma once
#include  "codegencontext.hpp"
namespace LIL {
	class LType
	{
	public:

		enum Kind
		{
			VOID,
			GENERICS,
			TYPE,
			INT,
			FP,
			CHAR,
			FUNTION,
			ARRAY,
			REF,
			RAWPOINTER,
			STRUCT
		};
		Kind kind;
		LLVMTypT LLVMTy;
		LTycpvT dep;

		LType();

		virtual std::string getID() const;

		virtual ~LType(){}

	protected:
		explicit LType(Kind kind, LTycpvT dep = LTycpvT());
		LType(Kind kind, LLVMTypT LLVMTy, LTycpvT dep = LTycpvT());
		LType(Kind kind, LLVMTypT LLVMTy, std::initializer_list<LType> deps);
	};

	class LGenTy :public LType
	{
	public:

		static LGenTy const* getGenTy(CodeGenC& context, LTycpsT dep);

		static LGenTy const* getGenTy(CodeGenC& context, LTycpvT dep);

		static LGenTy const* getGenTy(CodeGenC& context, std::initializer_list<LTycpT> dep);

		std::string getID() const override;

	protected:
		LGenTy(CodeGenC& context, LTycpsT dep);
		LGenTy(CodeGenC& context, LTycpvT dep);
		LGenTy(CodeGenC& context, std::initializer_list<LTycpT> dep);
	};
	class LTypeTy :public LType
	{
	public:

		static LTypeTy const* getTypeTy(CodeGenC& context, LTycpT dep);

		std::string getID() const override;

	protected:
		LTypeTy(CodeGenC& context, LTycpT dep);
	};
	class LIntTy :public LType
	{
	public:
		uint16_t bitwidth;
		bool is_signed;

		static LIntTy const* getIntTy(CodeGenC& context, uint16_t bitwidth, bool is_signed);

		static LIntTy const* getInt1Ty(CodeGenC& context);

		static LIntTy const* getInt8Ty(CodeGenC& context);

		static LIntTy const* getInt16Ty(CodeGenC& context);

		static LIntTy const* getInt32Ty(CodeGenC& context);

		static LIntTy const* getInt64Ty(CodeGenC& context);

		static LIntTy const* getInt128Ty(CodeGenC& context);

		static LIntTy const* getUInt8Ty(CodeGenC& context);

		static LIntTy const* getUInt16Ty(CodeGenC& context);

		static LIntTy const* getUInt32Ty(CodeGenC& context);

		static LIntTy const* getUInt64Ty(CodeGenC& context);

		static LIntTy const* getUInt128Ty(CodeGenC& context);

		bool canContain(const LIntTy const* source) const;

		std::string getID() const override;

	protected:
		LIntTy(CodeGenC& context, uint16_t bitwidth, bool is_signed);
	};

	class LFPTy :public LType
	{
	public:
		LIL::FPBitwidth bitwidth;

		static LFPTy const* getFPTy(CodeGenC& context, LIL::FPBitwidth bitwidth);

		static LFPTy const* getF16Ty(CodeGenC& context);

		static LFPTy const* getF32Ty(CodeGenC& context);

		static LFPTy const* getF64Ty(CodeGenC& context);

		static LFPTy const* getF80Ty(CodeGenC& context);

		static LFPTy const* getF128Ty(CodeGenC& context);

		bool canContain(const LFPTy const* source) const;
		
		std::string getID() const override;

	protected:
		LFPTy(CodeGenC& context, LIL::FPBitwidth bitwidth);
	};

	class LCharTy :public LType
	{
	public:
		LIL::charBitwidth bitwidth;
		bool is_signed;

		static LCharTy const* getCharTy(CodeGenC& context, LIL::charBitwidth bitwidth);

		static LCharTy const* getChar8Ty(CodeGenC& context);

		static LCharTy const* getChar16Ty(CodeGenC& context);

		static LCharTy const* getChar32Ty(CodeGenC& context);

		static LCharTy const* getChar64Ty(CodeGenC& context);

		static LCharTy const* getUChar8Ty(CodeGenC& context);

		static LCharTy const* getUChar16Ty(CodeGenC& context);

		static LCharTy const* getUChar32Ty(CodeGenC& context);

		static LCharTy const* getUChar64Ty(CodeGenC& context);

		bool canContain(const LCharTy const* source) const;
		
		std::string getID() const override;

	protected:
		LCharTy(CodeGenC& context, LIL::charBitwidth bitwidth);
	};

	class LFunctionTy :public LType
	{
	public:
		bool is_var_arg;

		static LFunctionTy const* getFunctionTy(CodeGenC& context, LTycpvT arg, LTycpT ret, bool is_var_arg);

		static LFunctionTy const* getFunctionTy(CodeGenC& context, LTycpvT arg_and_ret, bool is_var_arg);

		std::string getID() const override;

	protected:
		LFunctionTy(CodeGenC& context, LTycpvT arg_and_ret, bool is_var_arg);
		LFunctionTy(CodeGenC& context, LTycpvT arg, LTycpT ret, bool is_var_arg);
	};

	class LArrayTy :public LType
	{
	public:
		uint64_t size;

		static LArrayTy const* getArrayTy(CodeGenC& context, LTycpT type, uint64_t size);

		std::string getID() const override;

	protected:
		LArrayTy(CodeGenC& context, LTycpT type, uint64_t size);
	};

	class LRefTy :public LType
	{
	public:

		static LRefTy const* getRefTy(CodeGenC& context, LTycpT type);

		std::string getID() const override;

	protected:
		LRefTy(CodeGenC& context, LTycpT type);
	};

	class LPointerTy :public LType
	{
	public:

		static LPointerTy const* getPointerTy(CodeGenC& context, LTycpT type);

		std::string getID() const override;

	protected:
		LPointerTy(CodeGenC& context, LTycpT type);
	};
	class LStructTy :public LType
	{
	public:

		std::string name;

		std::vector<std::pair<LTycpT, accessSpecifier>> fields;

		static LStructTy const* setStructTy(CodeGenC& context, std::string name, std::vector<std::pair<LTycpT, accessSpecifier>> type);

		static LStructTy const* getStructTy(CodeGenC& context, std::string name);

		static LStructTy const* getStructTy(CodeGenC& context, LTycpvT type);

		std::string getID() const override;

	protected:
		LStructTy(CodeGenC& context, std::string name, std::vector<std::pair<LTycpT, accessSpecifier>> type);
		LStructTy(CodeGenC& context, LTycpvT type);
	};
}