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
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>


class Generator;

extern size_t yyline;

struct Statement
{
    virtual ~Statement() = default;
	virtual llvm::Value * accept(Generator * generator) = 0;
};

struct Expression
{
    virtual ~Expression() = default;
	virtual llvm::Value * accept(Generator * generator) = 0;
};

struct Type
{
	std::string name;

	Type(std::string name) : name(name) {}
    virtual ~Type() = default;

	static std::shared_ptr<Type> create(std::string name)
	{
		return std::make_shared<Type>(name);
	}

	static std::shared_ptr<Type> createVoid()
	{
		return std::make_shared<Type>("void");
	}

	virtual llvm::Type * getType()
	{
		if (name == "int") {
			return llvm::Type::getInt32Ty(llvm::getGlobalContext());
		}

		if (name == "float") {
			return llvm::Type::getFloatTy(llvm::getGlobalContext());
		}

		return llvm::Type::getVoidTy(llvm::getGlobalContext());
	}

	llvm::Value * getDefaultValue(std::shared_ptr<llvm::LLVMContext> context)
	{
		if (name == "int") {
			return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0, true);
		}

		if (name == "float") {
			return llvm::ConstantFP::get(llvm::Type::getFloatTy(*context), 0);
		}

		return nullptr;
	}

	virtual bool isArray()
	{
		return false;
	}
};

struct Parameter
{
	std::string name;
	std::shared_ptr<Type> type;

	Parameter(std::string name, std::shared_ptr<Type> type) : name(name), type(type) {}
    virtual ~Parameter() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct Block
{
    virtual ~Block() = default;

	std::vector<std::shared_ptr<Statement>> statements;
	bool returns = false;

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
		name(name), version(version), parameters(parameters), type(type), block(block) {}

	Function(std::string name, std::string version, std::vector<std::shared_ptr<Parameter>> parameters,
			std::shared_ptr<Block> block) :
		Function(name, version, parameters, Type::createVoid(), block) {}

    virtual ~Function() = default;

	std::string getVirtualName()
	{
		if (version.empty()) {
			return name;
		}

		return name + "." + version;
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

    virtual ~VarDecl() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct ExpressionStatement : Statement
{
	std::shared_ptr<Expression> expression;

	ExpressionStatement(std::shared_ptr<Expression> expression) : expression(expression) {}

    virtual ~ExpressionStatement() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct Assignment : Statement
{
	std::string variable;
	std::shared_ptr<Expression> expression;

	Assignment(std::string variable, std::shared_ptr<Expression> expression) :
		variable(variable), expression(expression) {}

    virtual ~Assignment() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct ArrayAssignment : Statement
{
	std::string variable;
	std::shared_ptr<Expression> index;
	std::shared_ptr<Expression> expression;

	ArrayAssignment(std::string variable, std::shared_ptr<Expression> index, std::shared_ptr<Expression> expression) :
		variable(variable), index(index), expression(expression) {}

    virtual ~ArrayAssignment() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct Return : Statement
{
	std::shared_ptr<Expression> expression;

	Return(std::shared_ptr<Expression> expression) : expression(expression) {}
	Return() : expression(std::shared_ptr<Expression>(nullptr)) {}

    virtual ~Return() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct If : Statement
{
	std::shared_ptr<Expression> condition;
	std::shared_ptr<Block> thenBlock;
	std::shared_ptr<Block> elseBlock;

	If(std::shared_ptr<Expression> condition, std::shared_ptr<Block> thenBlock, std::shared_ptr<Block> elseBlock) :
		condition(condition), thenBlock(thenBlock), elseBlock(elseBlock) {}

    virtual ~If() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct Print : Statement
{
	std::shared_ptr<Expression> expression;

	Print(std::shared_ptr<Expression> expression) :
		expression(expression) {}

    virtual ~Print() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct BinaryOp : Expression
{
	std::shared_ptr<Expression> left;
	std::shared_ptr<Expression> right;
	std::string op;

	BinaryOp(std::shared_ptr<Expression> left, std::string op, std::shared_ptr<Expression> right) :
		left(left), right(right), op(op) {}

    virtual ~BinaryOp() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct FunctionCall : Expression
{
	std::string name;
	std::string version;
	std::vector<std::shared_ptr<Expression>> arguments;

	FunctionCall(std::string name, std::string version, std::vector<std::shared_ptr<Expression>> arguments) :
		name(name), version(version), arguments(arguments) {}

    virtual ~FunctionCall() = default;

	std::string getVirtualName()
	{
		if (version.empty()) {
			return name;
		}

		return name + "." + version;
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

    virtual ~VersionInv() = default;

	virtual llvm::Value * accept(Generator * generator);

	std::string getVirtualName(std::string name)
	{
		if (version.empty()) {
			return name;
		}

		return name + "." + version;
	}
};

struct String : Expression
{
	std::string value;

	String(std::string value) : value(value) {}
    virtual ~String() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct Identifier : Expression
{
	std::string name;

	Identifier(std::string name) : name(name) {}
    virtual ~Identifier() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct Integer : Expression
{
	int value;

	Integer(int value) : value(value) {}
    virtual ~Integer() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct Float : Expression
{
	float value;

	Float(float value) : value(value) {}
    virtual ~Float() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct ArrayType : Type
{
	std::shared_ptr<Expression> size;
	bool isParameter = false;

	ArrayType(std::string name, std::shared_ptr<Expression> size) : Type(name), size(size) {}
	ArrayType(std::string name) : ArrayType(name, std::make_shared<Integer>(1)) {}

    virtual ~ArrayType() = default;

	virtual llvm::Type * getType()
	{
		auto type = Type::getType();

		if (isParameter) {
			return llvm::PointerType::getUnqual(type);
		}

		return type;
	}

	virtual bool isArray()
	{
		return true;
	}
};

struct Array : Expression
{
	std::vector<std::shared_ptr<Expression>> elements;

	Array(std::vector<std::shared_ptr<Expression>> elements) : elements(elements) {}

    virtual ~Array() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct ArrayIndex : Expression
{
	std::string name;
	std::shared_ptr<Expression> expression;

	ArrayIndex(std::string name, std::shared_ptr<Expression> expression) :
		name(name), expression(expression) {}

    virtual ~ArrayIndex() = default;

	virtual llvm::Value * accept(Generator * generator);
};

struct For : Statement
{
	std::string variable;
	std::shared_ptr<Expression> initial;
	std::shared_ptr<Expression> condition;
	std::shared_ptr<Expression> increment;
	std::shared_ptr<Block> block;

	For(std::string variable, std::shared_ptr<Expression> initial, std::shared_ptr<Expression> condition,
			std::shared_ptr<Block> block, std::shared_ptr<Expression> increment) :
		variable(variable), initial(initial), condition(condition), increment(increment), block(block) {}

	For(std::string variable, std::shared_ptr<Expression> initial, std::shared_ptr<Expression> condition,
			std::shared_ptr<Block> block) :
		For(variable, initial, condition, block, std::make_shared<Integer>(1)) {}

    virtual ~For() = default;

	virtual llvm::Value * accept(Generator * generator);
};
