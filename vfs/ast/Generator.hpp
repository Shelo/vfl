#pragma once

#include <iostream>

#include "../context/Scope.hpp"
#include "SyntaxTree.hpp"


class Generator
{
private:	
	std::shared_ptr<llvm::LLVMContext> context;
	
	std::shared_ptr<llvm::Module> module;
	
	llvm::IRBuilder<> builder;
	
	llvm::Function * lastFunction;
	
	llvm::BasicBlock * lastBlock;
	
	std::vector<Scope> scopes;

public:
	Generator() : context(std::shared_ptr<llvm::LLVMContext>(&llvm::getGlobalContext())),
		module(new llvm::Module("main", *context)), builder(*context) {}
	
	void generate(std::vector<std::shared_ptr<Function>> program);
	
	void dump()
	{
		module->dump();
	}
	
	void createScope()
	{
		scopes.push_back(Scope());
	}
	
	Scope & scope()
	{
		return scopes.back();
	}
	
	void popScope()
	{
		scopes.pop_back();
	}
	
	llvm::Value * getSymbol(std::string name)
	{
		llvm::Value * value;
		for (int i = scopes.size() - 1; i >= 0; i--) {
			value = scopes[i].get(name);
			
			if (value != nullptr) {
				return value;
			}
		}
		
		throw "Unknown symbol: " + name;
	}

	llvm::Value * visit(Block & node);
	llvm::Value * visit(Function & node);
	llvm::Value * visit(Parameter & node);
	llvm::Value * visit(VarDecl & node);
	llvm::Value * visit(VersionInv & node);
	llvm::Value * visit(FunctionCall & node);
	llvm::Value * visit(Float & node);
	llvm::Value * visit(Integer & node);
	llvm::Value * visit(String & node);
	llvm::Value * visit(BinaryOp & node);
	llvm::Value * visit(Identifier & node);
	llvm::Value * visit(Return & node);
	llvm::Value * visit(ExpressionStatement & node);
	llvm::Value * visit(Assignment & node);
	llvm::Value * visit(If & node);
};
