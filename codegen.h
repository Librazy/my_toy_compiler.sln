#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace llvm;

class NBlock;

class CodeGenBlock
{
public:
	BasicBlock* block;
	Value* returnValue;
	std::string name;
	bool isTranspent = false;
	bool isFunction = false;
	std::map<std::string, Value*> locals;
};

class CodeGenContext
{
	std::vector<CodeGenBlock *> blocks;
	Function* mainFunction;
public:
	raw_os_ostream llclog{std::clog};
	Module* module;
	LLVMContext llvmContext;
	std::map<std::string, Function*> globalFun;
	std::vector<std::string> funcBlocks;
	std::map<std::string, std::vector<std::string>> extra;

	CodeGenContext(): mainFunction(nullptr), llvmContext()
	{
		module = new Module("main", llvmContext);
		std::vector<Type *> powf_a_t;
		powf_a_t.push_back(Type::getDoubleTy(llvmContext));
		powf_a_t.push_back(Type::getDoubleTy(llvmContext));
		registeGlobalIntrinsic(Intrinsic::pow, "llvm.pow.f64", "pow", powf_a_t);
	}

	void registeGlobalIntrinsic(Intrinsic::ID intid, std::string name, std::string id, ArrayRef<Type*> Tys = None)
	{
		auto powf = getDeclaration(module, intid, makeArrayRef(Tys));
		powf->setName(name);
		globalFun.insert(std::pair<std::string, Function*>(id, powf));
	}

	void generateCode(NBlock& root);
	GenericValue runCode() const;

	std::map<std::string, Value*>& locals() const
	{
		return blocks.back()->locals;
	}

	Value* find_locals(std::string const& s)
	{
		auto i = blocks.end();
		auto througthFun = false;
		std::clog << "start finding " + s << std::endl;
		do {
			if (i != blocks.begin())--i;
			std::clog << "    finding local " + s + " in bb " + (*i)->name + " " << &(**i) << std::endl;
			std::clog << "      " << (i != blocks.begin()) << " " << ((*i)->isTranspent) << std::endl;
			if ((*i)->locals.find(s) != (*i)->locals.end()) {
				if (througthFun) {
					auto& ex = extra[ftrace(1) + "__" + funcBlocks.back()];
					if (std::find(ex.begin(), ex.end(), s) == ex.end())
						ex.push_back(s);
				}
				std::clog << "    found local " << (*i)->locals[s] << " for " << s << std::endl;
				(*i)->locals[s]->getType()->print(llclog);
				(llclog << "\n").flush();
				return (*i)->locals[s];
			}
			if ((*i)->isFunction) {
				if (!througthFun) {
					througthFun = true;
				} else {
					std::cerr << "double local fun " + (*i)->name + " " + s << std::endl;
					assert(false);
				}
			}
		} while ((*i)->isTranspent && i != blocks.begin());
		return nullptr;
	}

	std::string trace()
	{
		std::string a("");
		for (auto b : blocks) { a += b->name + "_"; }
		return a;
	}

	std::string ftrace(int r = 0)
	{
		std::string a("");
		auto n = funcBlocks.size() - r;
		for (auto const& s : funcBlocks) {
			a += s + "_";
			if (!(--n)) { break; }
		}
		return a;
	}

	BasicBlock* currentBlock() { return blocks.back()->block; }

	void pushBlock(BasicBlock* block, std::string name, bool transpent = false, bool function = false)
	{
		std::clog << "    pushing bb " + name + " " << transpent << " " << block << std::endl;
		blocks.push_back(new CodeGenBlock());
		blocks.back()->returnValue = nullptr;
		blocks.back()->block = block;
		blocks.back()->name = name;
		blocks.back()->isTranspent = transpent;
		blocks.back()->isFunction = function;
	}

	void popBlock()
	{
		auto top = blocks.back();
		std::clog << "    poping bb " + top->name + " " << top->isTranspent << " " << top->block << std::endl;
		blocks.pop_back();
		delete top;
	}

	void popBlockUntil(BasicBlock* b)
	{
		while (blocks.back()->block != b) {
			auto top = blocks.back();
			blocks.pop_back();
			delete top;
		}
	}

	void setCurrentReturnValue(Value* value) { blocks.back()->returnValue = value; }
	Value* getCurrentReturnValue() { return blocks.back()->returnValue; }
};
