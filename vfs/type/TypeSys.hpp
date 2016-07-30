#ifndef VFS_TYPESYS_HPP
#define VFS_TYPESYS_HPP

#include <map>
#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>

class TypeSys
{
private:
    std::map<llvm::Type*, std::map<llvm::Type*, llvm::Type*>> coerceTab;
    std::map<llvm::Type*, std::map<llvm::Type*, llvm::CastInst::CastOps>> castTab;
    std::map<std::pair<llvm::Type*, std::string>, llvm::Instruction::BinaryOps> opTab;

    llvm::Type * floatTy = llvm::Type::getFloatTy(llvm::getGlobalContext());
    llvm::Type * intTy = llvm::Type::getInt32Ty(llvm::getGlobalContext());
    llvm::Type * doubleTy = llvm::Type::getDoubleTy(llvm::getGlobalContext());

public:
    TypeSys();

    void add(llvm::Type * l, llvm::Type * r, llvm::Type * result);
    void addCast(llvm::Type * from, llvm::Type * to, llvm::CastInst::CastOps op);
    void addOp(llvm::Type * type, std::string op, llvm::Instruction::BinaryOps llvmOp);

    llvm::Type * coerce(llvm::Type * l, llvm::Type * r);

    llvm::Value * cast(llvm::Value * value, llvm::Type * type, llvm::BasicBlock * block);

    llvm::Instruction::BinaryOps getOp(llvm::Type *type, std::string op);

    llvm::CastInst::CastOps getCastOp(llvm::Type * from, llvm::Type * to);
};

#endif //VFS_TYPESYS_HPP
