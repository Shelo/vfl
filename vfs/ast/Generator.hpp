#pragma once

#include <iostream>
#include <map>

#include "../context/Scope.hpp"
#include "SyntaxTree.hpp"
#include "../type/TypeSys.hpp"


class Generator
{
private:
	std::shared_ptr<llvm::LLVMContext> context;
	
	std::shared_ptr<llvm::Module> module;
	
	llvm::IRBuilder<> builder;
	
	Function * lastFunction;
	
	std::vector<std::shared_ptr<Scope>> scopes;

	TypeSys typeSys;

    std::map<std::string, llvm::Function*> funcAlias;

public:
	Generator() : context(std::shared_ptr<llvm::LLVMContext>(&llvm::getGlobalContext())),
		module(new llvm::Module("main", *context)), builder(*context) {}

	void generate(std::vector<std::shared_ptr<Function>> program,
            std::vector<std::shared_ptr<Struct>> structs);

	void dump()
	{
		module->dump();
	}
	
	void createScope()
	{
		Scope * parent = nullptr;

        if (!scopes.empty()) {
            parent = scopes.back().get();
        }

		scopes.push_back(std::make_shared<Scope>(parent));
	}
	
	Scope & scope()
	{
		return *scopes.back();
	}

	void popScope()
	{
		scopes.pop_back();
	}

	llvm::Value * visit(Block & node);
    llvm::Value * visit(Struct & node);
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
	llvm::Value * visit(Print & node);
	llvm::Value * visit(Array & node);
	llvm::Value * visit(ArrayIndex & node);
    llvm::Value * visit(StructMember & node);
	llvm::Value * visit(ArrayAssignment & node);
	llvm::Value * visit(StructAssignment & node);
	llvm::Value * visit(For & node);
	llvm::Value * visit(Bool & node);
};
