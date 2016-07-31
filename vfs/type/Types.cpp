#include "Types.hpp"


std::shared_ptr<Type> Type::createVoid()
{
    return std::make_shared<Type>("void");
}

llvm::Type * Type::getType(TypeSys & typeSys)
{
    if (name == "int") {
        return typeSys.intTy;
    }

    if (name == "float") {
        return typeSys.floatTy;
    }

    if (name == "string") {
        return typeSys.stringTy;
    }

    if (name == "bool") {
        return typeSys.boolTy;
    }

    return typeSys.voidTy;
}

llvm::Value * Type::getDefaultValue(std::shared_ptr<llvm::LLVMContext> context)
{
    if (name == "int") {
        return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), 0, true);
    }

    if (name == "float") {
        return llvm::ConstantFP::get(llvm::Type::getFloatTy(*context), 0);
    }

    return nullptr;
}

llvm::Type * ArrayType::getType(TypeSys & typeSys)
{
    return llvm::PointerType::get(Type::getType(typeSys), 0);
}

llvm::Type * StructType::getType(TypeSys & typeSys)
{
    return llvm::PointerType::get(typeSys.getStructType(name), 0);
}
