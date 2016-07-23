#pragma once

#include <memory>
#include <vector>
#include <string>

#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>


class Generator;

extern size_t yyline;

struct Statement
{
	virtual llvm::Value * accept(Generator * generator) = 0;
};

struct Expression
{
	virtual llvm::Value * accept(Generator * generator) = 0;
};

struct Type
{
	std::string name;

	Type(std::string name) : name(name) {}
	
	static std::shared_ptr<Type> create(std::string name)
	{
		return std::make_shared<Type>(name);
	}
	
	static std::shared_ptr<Type> createVoid()
	{
		return std::make_shared<Type>("void");
	}
	
	llvm::Type * getType()
	{
		if (name == "int") {
			return llvm::Type::getInt64Ty(llvm::getGlobalContext());
		}

		if (name == "float") {
			return llvm::Type::getFloatTy(llvm::getGlobalContext());
		}

		return llvm::Type::getVoidTy(llvm::getGlobalContext());
	}
};

struct Parameter
{
	std::string name;
	std::shared_ptr<Type> type;
	
	Parameter(std::string name, std::shared_ptr<Type> type) : name(name), type(type) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct Block
{
	std::vector<std::shared_ptr<Statement>> statements;
	bool entry = false;
	
	virtual llvm::Value * accept(Generator * generator);
};

struct Function
{
	std::string name;
	std::string version;
	std::vector<std::shared_ptr<Parameter>> parameters;
	std::shared_ptr<Type> type;
	std::shared_ptr<Block> block;
	
	Function(std::string name, std::string version, std::vector<std::shared_ptr<Parameter>> parameters,
			std::shared_ptr<Type> type, std::shared_ptr<Block> block) :
		name(name), version(version), parameters(parameters), block(block), type(type)
	{
		block->entry = true;
	}

	Function(std::string name, std::string version, std::vector<std::shared_ptr<Parameter>> parameters,
			std::shared_ptr<Block> block) :
		Function(name, version, parameters, Type::createVoid(), block) {}
	
	std::string getVirtualName()
	{
		if (version.empty()) {
			return name;
		}

		return name + "__" + version;
	}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct VarDecl : Statement
{
	std::string name;
	std::shared_ptr<Type> type;
	std::shared_ptr<Expression> expression;
	
	VarDecl(std::string name, std::shared_ptr<Type> type, std::shared_ptr<Expression> expression) :
		name(name), type(type), expression(expression) {}
	
	VarDecl(std::string name, std::shared_ptr<Type> type) : name(name), type(type) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct ExpressionStatement : Statement
{
	std::shared_ptr<Expression> expression;
	
	ExpressionStatement(std::shared_ptr<Expression> expression) : expression(expression) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct Assignment : Statement
{
	std::string variable;
	std::shared_ptr<Expression> expression;
	
	Assignment(std::string variable, std::shared_ptr<Expression> expression) :
		variable(variable), expression(expression) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct Return : Statement
{
	std::shared_ptr<Expression> expression;
	
	Return(std::shared_ptr<Expression> expression) : expression(expression) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct If : Statement
{
	std::shared_ptr<Expression> condition;
	std::shared_ptr<Block> block;
	
	If(std::shared_ptr<Expression> condition, std::shared_ptr<Block> block) :
		condition(condition), block(block) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct BinaryOp : Expression
{
	std::shared_ptr<Expression> left;
	std::shared_ptr<Expression> right;
	std::string op;
	
	BinaryOp(std::shared_ptr<Expression> left, std::string op, std::shared_ptr<Expression> right) : 
		left(left), op(op), right(right) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct FunctionCall : Expression
{
	std::string name;
	std::string version;
	std::vector<std::shared_ptr<Expression>> arguments;
	
	FunctionCall(std::string name, std::string version, std::vector<std::shared_ptr<Expression>> arguments) : 
		name(name), version(version), arguments(arguments) {}
	
	std::string getVirtualName()
	{
		if (version.empty()) {
			return name;
		}

		return name + "__" + version;
	}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct VersionInv : Expression
{
	std::string name;
	std::string version;
	std::vector<std::shared_ptr<Expression>> arguments;
	
	VersionInv(std::string version, std::vector<std::shared_ptr<Expression>> arguments) : 
		version(version), arguments(arguments) {}
	
	virtual llvm::Value * accept(Generator * generator);
	
	std::string getVirtualName(std::string name)
	{
		if (version.empty()) {
			return name;
		}

		return name + "__" + version;
	}
};

struct String : Expression
{
	std::string value;
	
	String(std::string value) : value(value) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct Identifier : Expression
{
	std::string name;
	
	Identifier(std::string name) : name(name) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct Integer : Expression
{
	int value;
	
	Integer(int value) : value(value) {}
	
	virtual llvm::Value * accept(Generator * generator);
};

struct Float : Expression
{
	float value;
	
	Float(float value) : value(value) {}
	
	virtual llvm::Value * accept(Generator * generator);
};