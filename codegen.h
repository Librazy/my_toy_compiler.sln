#pragma once
#include <iostream>
#include <vector>
#include "debug_stream.hpp"
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
	BasicBlock* block = nullptr;
	Value* returnValue = nullptr;
	bool isTranspent = false;
	bool isFunction = false;
	std::string name;
	std::map<std::string, Value*> locals;
};

class CodeGenContext
{
	std::vector<CodeGenBlock *> blocks;
	Function* mainFunction;
public:
	raw_os_ostream llclog{std::clog};
	debug_stream dclog;
	Module* module;
	LLVMContext llvmContext;
	std::map<std::string, Function*> globalFun;
	std::vector<std::string> funcBlocks;
	std::map<std::string, std::vector<std::string>> extra;

	CodeGenContext(): mainFunction(nullptr), dclog("Debug", debug_stream::verbose, std::clog), llvmContext()
	{
		module = new Module("main", llvmContext);
		std::vector<Type *> powfArgumentTypes;
		powfArgumentTypes.push_back(Type::getDoubleTy(llvmContext));
		powfArgumentTypes.push_back(Type::getDoubleTy(llvmContext));
		registeGlobalIntrinsic(Intrinsic::pow, "llvm.pow.f64", "pow", powfArgumentTypes);
	}

	void registeGlobalIntrinsic(Intrinsic::ID intid, std::string name, std::string id, ArrayRef<Type*> Tys = None)
	{
		auto powf = getDeclaration(module, intid, makeArrayRef(Tys));
		powf->setName(name);
		globalFun.insert(std::pair<std::string, Function*>(id, powf));
	}

	void generateCode(NBlock& root);
	GenericValue runCode();

	std::map<std::string, Value*>& locals() const
	{
		return blocks.back()->locals;
	}

	Value* find_locals(std::string const& s)
	{
		auto i = blocks.end();
		auto througthFun = 0;
		dclog << debug_stream::verbose << "start finding " + s << std::endl;
		dclog << debug_stream::indent(2, +1);
		do {
			if (i != blocks.begin())--i;
			dclog << "finding " + s + " in block " << (*i)->name << ", transpent " << (*i)->isTranspent << ", addr " << *i << std::endl;
			if ((*i)->locals.find(s) != (*i)->locals.end()) {
				if (througthFun) {
					auto level = througthFun + 1;
					while (--level) {
						auto& ex = extra[ftrace(level) + "__" + funcBlocks.back()];
						if (std::find(ex.begin(), ex.end(), s) == ex.end()) {
							dclog << "marking " + s + " as extra in " << ftrace(level) + "__" + funcBlocks.back() << std::endl;
							ex.push_back(s);
						}
					}
				}
				(dclog << "found " << s << " at " << (*i)->locals[s] << " in block " << (*i)->name << ", type ").flush();
				if (dclog.max_level >= debug_stream::verbose) {
					(*i)->locals[s]->getType()->print(llclog);
					(llclog << "\n").flush();
				}
				dclog << debug_stream::indent(2, -1);
				return (*i)->locals[s];
			}
			if ((*i)->isFunction) {
				througthFun += 1;
			}
		} while ((*i)->isTranspent && i != blocks.begin());
		dclog << debug_stream::indent(2, -1);
		dclog << debug_stream::error << "can't find " << s;
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
		dclog << debug_stream::verbose << "pushing basic block " + name + ", transpent:" << transpent << ", addr:" << block << std::endl;
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
		dclog << debug_stream::verbose << "poping block " + top->name << ", " + top->block->getName().str() << ", transpent:" << top->isTranspent << ", addr:" << top->block << std::endl;
		blocks.pop_back();
		delete top;
	}

	void popBlockUntil(BasicBlock* b)
	{
		dclog << debug_stream::verbose << "poping blocks..." << std::endl;
		dclog << debug_stream::indent(2, +1);
		while (blocks.back()->block != b) {
			auto top = blocks.back();
			dclog << debug_stream::verbose << "poping block " + top->name << ", " + top->block->getName().str() << ", transpent:" << top->isTranspent << ", addr:" << top->block << std::endl;
			blocks.pop_back();
			delete top;
		}
		dclog << debug_stream::indent(2, -1);
		dclog << "until basic block " << b << std::endl;
	}

	void setCurrentReturnValue(Value* value) { blocks.back()->returnValue = value; }
	Value* getCurrentReturnValue() { return blocks.back()->returnValue; }
};
