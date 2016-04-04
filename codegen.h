#include <stack>
#include <vector>
#include <queue>
#include <string>
#include <typeinfo>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

class NBlock;

class CodeGenBlock {
public:
    BasicBlock *block;
    Value *returnValue;
	std::string name;
	bool isTranspent = false;
    std::map<std::string, Value*> locals;
};

class CodeGenContext {
    std::vector<CodeGenBlock *> blocks;
    Function *mainFunction;
	
public:
    Module *module;
	std::map<std::string, Function*> globalFun;
    CodeGenContext():mainFunction(nullptr)
    {
	    module = new Module("main", getGlobalContext());
		std::vector<Type *> powf_a_t;
		powf_a_t.push_back(Type::getDoubleTy(getGlobalContext()));
		powf_a_t.push_back(Type::getDoubleTy(getGlobalContext()));
		registeGloIntFun(Intrinsic::pow, "llvm.pow.f64", "pow", powf_a_t);
    }

	void registeGloIntFun(llvm::Intrinsic::ID intid, std::string name, std::string id, llvm::ArrayRef<llvm::Type*> Tys = None)
	{
		Function* powf = getDeclaration(module, intid, makeArrayRef(Tys)); powf->setName(name);
		globalFun.insert(std::pair<std::string, Function*>(id, powf));
	}
    void generateCode(NBlock& root);
    GenericValue runCode() const;
    std::map<std::string, Value*>& locals() { return blocks.back()->locals; }
    Value* find_locals(std::string& s)
    {
		auto i = blocks.end();
		std::clog << "start finding " + s << std::endl;
		do {
			if (i != blocks.begin())--i;
			std::clog << "    finding local "+s+" in bb "+(*i)->name +" "<<&(**i)<< std::endl;
			std::clog << "      " << (i != blocks.begin()) << " " << ((*i)->isTranspent) << std::endl;
			if ((*i)->locals.find(s) != (*i)->locals.end()) {
				return (*i)->locals[s];
			}
		} while ((*i)->isTranspent&&i != blocks.begin());
		return nullptr;
    }
	std::string trace()
	{
		std::string a("");
		auto i = blocks.end();
		do {
			if (i != blocks.begin())--i;
			a+=(*i)->name+ "_";
		} while (i != blocks.begin());
		return a;
	}
    BasicBlock *currentBlock() { return blocks.back()->block; }
	void pushBlock(BasicBlock *block,std::string name , bool trans = false) {
		std::clog << "    pushing bb " + name + " " << trans << " " <<  block << std::endl;
		blocks.push_back(new CodeGenBlock()); 
		blocks.back()->returnValue = nullptr;
		blocks.back()->block = block;
		blocks.back()->name = name;
		blocks.back()->isTranspent = trans;
	}
	void popBlock() {
		CodeGenBlock *top = blocks.back(); blocks.pop_back(); delete top;
	}
	void popTBlock() { while (blocks.back()->isTranspent) { CodeGenBlock *top = blocks.back(); blocks.pop_back(); delete top; } }
	void popBlockUntil(BasicBlock* b) { while (blocks.back()->block!=b) { CodeGenBlock *top = blocks.back(); blocks.pop_back(); delete top; } }

    void setCurrentReturnValue(Value *value) { blocks.back()->returnValue = value; }
    Value* getCurrentReturnValue() { return blocks.back()->returnValue; }
};
