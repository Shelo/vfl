#include "SyntaxTree.hpp"
#include "Generator.hpp"

llvm::Value * Block::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * FunctionCall::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * VarDecl::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * BinaryOp::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Parameter::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Return::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Identifier::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Integer::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Float::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * String::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * VersionInv::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * If::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Print::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Assignment::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Function::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * ExpressionStatement::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Array::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * ArrayIndex::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * ArrayAssignment::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * For::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * Bool::accept(Generator * generator)
{
	return generator->visit(*this);
}

llvm::Value * PrintFormat::accept(Generator * generator)
{
    return generator->visit(*this);
}
