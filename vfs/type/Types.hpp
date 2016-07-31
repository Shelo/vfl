#ifndef VFS_TYPES_HPP
#define VFS_TYPES_HPP

#include <map>

#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Constants.h>

#include "../ast/SyntaxTree.hpp"
#include "TypeSys.hpp"


struct Type
{
    std::string name;

    Type(std::string name) : name(name) {}
    virtual ~Type() = default;

    static std::shared_ptr<Type> createVoid();

    virtual llvm::Type * getType(TypeSys & typeSys);

    llvm::Value * getDefaultValue(std::shared_ptr<llvm::LLVMContext> context);

    virtual bool isArray()
    {
        return false;
    }

    virtual bool isStruct()
    {
        return false;
    }
};


struct ArrayType : Type
{
    std::shared_ptr<Expression> size;

    ArrayType(std::string name, std::shared_ptr<Expression> size = std::make_shared<Integer>(1)) :
            Type(name), size(size) {}

    virtual ~ArrayType() = default;

    virtual llvm::Type * getType(TypeSys & typeSys);

    virtual bool isArray()
    {
        return true;
    }
};


struct StructType : Type
{
    StructType(std::string name) : Type(name) {}

    virtual ~StructType() = default;

    virtual llvm::Type * getType(TypeSys & typeSys);

    virtual bool isStruct()
    {
        return true;
    }
};


#endif //VFS_TYPES_HPP
