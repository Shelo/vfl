#include <llvm/IR/Value.h>
#include <llvm/IR/InstrTypes.h>
#include "TypeSys.hpp"

TypeSys::TypeSys()
{
    addCoercion(intTy, floatTy, floatTy);

    addCast(intTy, floatTy, llvm::CastInst::SIToFP);
    addCast(intTy, doubleTy, llvm::CastInst::SIToFP);
    addCast(boolTy, doubleTy, llvm::CastInst::SIToFP);
    addCast(floatTy, doubleTy, llvm::CastInst::FPExt);
    addCast(floatTy, intTy, llvm::CastInst::FPToSI);
    addCast(doubleTy, intTy, llvm::CastInst::FPToSI);

    addOp(intTy, "+", llvm::Instruction::Add);
    addOp(floatTy, "+", llvm::Instruction::FAdd);
    addOp(doubleTy, "+", llvm::Instruction::FAdd);

    addOp(intTy, "-", llvm::Instruction::Sub);
    addOp(floatTy, "-", llvm::Instruction::FSub);
    addOp(doubleTy, "-", llvm::Instruction::FSub);

    addOp(intTy, "*", llvm::Instruction::Mul);
    addOp(floatTy, "*", llvm::Instruction::FMul);
    addOp(doubleTy, "*", llvm::Instruction::FMul);

    addOp(intTy, "/", llvm::Instruction::SDiv);
    addOp(floatTy, "/", llvm::Instruction::FDiv);
    addOp(doubleTy, "/", llvm::Instruction::FDiv);

    addOp(intTy, "%", llvm::Instruction::SRem);
    addOp(floatTy, "%", llvm::Instruction::FRem);
    addOp(doubleTy, "%", llvm::Instruction::FRem);
}

void TypeSys::addCoercion(llvm::Type *l, llvm::Type *r, llvm::Type *result)
{
    if (coerceTab.find(l) == coerceTab.end()) {
        coerceTab[l] = std::map<llvm::Type*, llvm::Type*>();
    }

    coerceTab[l][r] = result;
}

void TypeSys::addCast(llvm::Type * from, llvm::Type * to, llvm::CastInst::CastOps op)
{
    if (castTab.find(from) == castTab.end()) {
        castTab[from] = std::map<llvm::Type*, llvm::CastInst::CastOps>();
    }

    castTab[from][to] = op;
}

void TypeSys::addOp(llvm::Type * type, std::string op, llvm::Instruction::BinaryOps llvmOp)
{
    mathOpTab[std::pair<llvm::Type*, std::string>(type, op)] = llvmOp;
}

llvm::CastInst::CastOps TypeSys::getCastOp(llvm::Type * from, llvm::Type * to)
{
    if (castTab.find(from) == castTab.end()) {
        throw std::runtime_error("Type cannot be casted: " + std::to_string(from->getTypeID()));
    }

    auto subIt = castTab[from].find(to);

    if (subIt != castTab[from].end()) {
        return castTab[from][to];
    }

    throw std::runtime_error("Unknown cast from " + std::to_string(from->getTypeID())
                             + " to " + std::to_string(to->getTypeID()));
}

llvm::Type * TypeSys::coerce(llvm::Type * l, llvm::Type * r)
{
    if (l == r) {
        return l;
    }

    auto it = coerceTab.find(l);

    if (it != coerceTab.end()) {
        auto it2 = it->second.find(r);

        if (it2 != it->second.end()) {
            return it2->second;
        }
    }

    it = coerceTab.find(r);

    if (it != coerceTab.end()) {
        auto it2 = it->second.find(l);

        if (it2 != it->second.end()) {
            return it2->second;
        }
    }

    throw std::runtime_error("No conversion between " + std::to_string(l->getTypeID())
                             + " and " + std::to_string(r->getTypeID()));
}

llvm::Value * TypeSys::cast(llvm::Value * value, llvm::Type * type, llvm::BasicBlock * block)
{
    if (value->getType() == type) {
        return value;
    }

    return llvm::CastInst::Create(getCastOp(value->getType(), type), value, type, "cast", block);
}

llvm::Instruction::BinaryOps TypeSys::getMathOp(llvm::Type *type, std::string op)
{
    return mathOpTab[std::pair<llvm::Type*, std::string>(type, op)];
}

bool TypeSys::isFP(llvm::Type * type)
{
    return type != intTy;
}

llvm::CmpInst::Predicate TypeSys::getCmpPredicate(llvm::Type * type, std::string op)
{
    if (type == intTy) {
        if (op == "==") {
            return llvm::CmpInst::Predicate::ICMP_EQ;
        } else if (op == "!=") {
            return llvm::CmpInst::Predicate::ICMP_NE;
        } else if (op == "<") {
            return llvm::CmpInst::Predicate::ICMP_SLT;
        } else if (op == ">") {
            return llvm::CmpInst::Predicate::ICMP_SGT;
        } else if (op == "<=") {
            return llvm::CmpInst::Predicate::ICMP_SLE;
        } else if (op == ">=") {
            return llvm::CmpInst::Predicate::ICMP_SGE;
        }
    } else {
        if (op == "==") {
            return llvm::CmpInst::Predicate::FCMP_OEQ;
        } else if (op == "!=") {
            return llvm::CmpInst::Predicate::FCMP_ONE;
        } else if (op == "<") {
            return llvm::CmpInst::Predicate::FCMP_OLT;
        } else if (op == ">") {
            return llvm::CmpInst::Predicate::FCMP_OGT;
        } else if (op == "<=") {
            return llvm::CmpInst::Predicate::FCMP_OLE;
        } else if (op == ">=") {
            return llvm::CmpInst::Predicate::FCMP_OGE;
        }
    }

    throw std::runtime_error("Not a comparison operator: " + op);
}

