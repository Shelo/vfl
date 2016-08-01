#include "Generator.hpp"

#include "../type/Types.hpp"


void Generator::generate(std::vector<std::shared_ptr<Function>> program,
        std::vector<std::shared_ptr<Struct>> structs)
{
    funcAlias["Print.format"] = llvm::dyn_cast<llvm::Function>(
            module->getOrInsertFunction("printf", llvm::FunctionType::get(
                    typeSys.intTy,
                    llvm::PointerType::get(typeSys.charTy, 0),
                    true
            )));

    for (auto s : structs) {
        s->accept(this);
    }

    for (auto f : program) {
        f->accept(this);
    }
}

llvm::Value * Generator::visit(Function & node)
{
    std::vector<llvm::Type *> parameterTypes;
    for (auto i : node.parameters) {
        parameterTypes.push_back(i->type->getType(typeSys));
    }

    std::string name = node.name;
    if (node.name == "Main") {
        name = "main";
    } else {
        name = node.getVirtualName();
    }

    auto type = llvm::FunctionType::get(node.type->getType(typeSys), parameterTypes, false);
    auto function = llvm::Function::Create(type, llvm::Function::ExternalLinkage, name, module.get());
    lastFunction = &node;

    // create the block for this function.
    builder.SetInsertPoint(llvm::BasicBlock::Create(*context, "entry", function));

    // create the scope.
    createScope();

    int i = 0;
    for (auto & arg : function->args()) {
        auto parameter = node.parameters[i];
        arg.setName("param." + parameter->name);

        auto value = parameter->accept(this);
        builder.CreateStore(&arg, value);

        scope().add(parameter->name, value);

        i++;
    }

    node.block->accept(this);

    if (builder.GetInsertBlock()->getTerminator() == nullptr) {
        builder.CreateRetVoid();
    }

    popScope();

    return function;
}

llvm::Value * Generator::visit(Parameter & parameter)
{
    return builder.CreateAlloca(parameter.type->getType(typeSys), nullptr, parameter.name);
}

llvm::Value * Generator::visit(Block & node)
{
    llvm::Value * last = nullptr;

    for (auto i : node.statements) {
        i->accept(this);
    }

    return last;
}

llvm::Value * Generator::visit(VarDecl & node)
{
    llvm::Value * initial = nullptr;

    if (node.expression != nullptr) {
        initial = node.expression->accept(this);
    }

    llvm::Value * value = nullptr;

    bool hasDefinedType = node.type != nullptr;

    llvm::Type * type = nullptr;

    if (node.type == nullptr) {
        if (initial == nullptr) {
            throw std::runtime_error("Variable type inference needs a definition.");
        }

        type = initial->getType();
    } else {
        type = node.type->getType(typeSys);
    }

    if (node.type && node.type->isArray()) {
        // if this is an array, we should allocate it and then fake it as an initial value.
        auto arrayType = reinterpret_cast<ArrayType*>(node.type.get());
        auto arraySize = arrayType->size->accept(this);
        initial = builder.CreateAlloca(type->getArrayElementType(), arraySize);
        type = initial->getType();

        // this flag has to be disabled, since arrays cannot be casted.
        hasDefinedType = false;
    }

    if (node.type && node.type->isStruct()) {
        // if this is a struct, we should allocate it and then fake it as an initial value.
        auto structType = typeSys.getStructType(node.type->name);
        initial = builder.CreateAlloca(structType);
        type = initial->getType();

        // this flag has to be disabled, since structs cannot be casted.
        hasDefinedType = false;
    }

    value = builder.CreateAlloca(type, nullptr, node.name);

    if (initial != nullptr) {
        if (hasDefinedType) {
            initial = typeSys.cast(initial, node.type->getType(typeSys), builder.GetInsertBlock());
        }

        builder.CreateStore(initial, value);
    }

    scope().add(node.name, value);
    return value;
}

llvm::Value * Generator::visit(Assignment & node)
{
    auto value = scope().get(node.variable);
    return builder.CreateStore(node.expression->accept(this), value);
}

llvm::Value * Generator::visit(ArrayAssignment & node)
{
    auto array = scope().get(node.variable);

    auto arrayLoad = builder.CreateLoad(array);
    auto value = node.expression->accept(this);
    auto index = node.index->accept(this);
    auto ptr = builder.CreateInBoundsGEP(arrayLoad, index);

    return builder.CreateStore(value, ptr);
}

llvm::Value * Generator::visit(VersionInv & node)
{
    auto name = (std::string) lastFunction->name;
    auto virtualName = node.getVirtualName(name);
    auto function = module->getFunction(virtualName);

    if (function == nullptr) {
        throw std::runtime_error("Function not defined: " + name);
    }

    // TODO: check for arg compatibility.

    std::vector<llvm::Value *> values;
    for (auto i : node.arguments) {
        values.push_back(i->accept(this));
    }

    return builder.CreateCall(function, values);
}

llvm::Value * Generator::visit(FunctionCall & node)
{
    auto function = module->getFunction(node.getVirtualName());

    if (function == nullptr) {
        if (funcAlias.find(node.getVirtualName()) == funcAlias.end()) {
            throw std::runtime_error("Function not defined: " + node.name);
        }

        function = funcAlias[node.getVirtualName()];
    }

    // TODO: check arg compatibility.

    std::vector<llvm::Value *> values;
    for (auto i : node.arguments) {
        auto value = i->accept(this);

        // variadic function promotion.
        if (function->isVarArg() && value->getType()->isFloatTy()) {
            value = typeSys.cast(value, typeSys.doubleTy, builder.GetInsertBlock());
        }

        values.push_back(value);
    }

    return builder.CreateCall(function, values);
}

llvm::Value * Generator::visit(Return & node)
{
    llvm::Value * returnValue = nullptr;

    if (node.expression) {
        returnValue = node.expression->accept(this);

        auto type = returnValue->getType();
        if (type->isVoidTy()) {
            returnValue = nullptr;
        } else if (type->getTypeID() == 14 || type->getTypeID() == 13) {
            returnValue = builder.CreateLoad(returnValue);
        }
    }

    return builder.CreateRet(returnValue);
}

llvm::Value * Generator::visit(ExpressionStatement & node)
{
    return node.expression->accept(this);
}

llvm::Value * Generator::visit(Identifier & node)
{
    auto value = scope().get(node.name);

    if (value == nullptr) {
        throw std::runtime_error("Symbol not defined: " + node.name);
    }

    return builder.CreateLoad(value);
}

llvm::Value * Generator::visit(Integer & node)
{
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), node.value, true);
}

llvm::Value * Generator::visit(Float & node)
{
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(*context), node.value);
}

llvm::Value * Generator::visit(String & node)
{
    auto constString = llvm::ConstantDataArray::getString(*context, node.value);
    auto var = new llvm::GlobalVariable(*module,
            llvm::ArrayType::get(llvm::IntegerType::get(*context, 8), node.value.length() + 1),
            true, llvm::GlobalValue::PrivateLinkage, constString, ".str");

    auto zero = llvm::Constant::getNullValue(typeSys.intTy);

    return builder.CreateInBoundsGEP(var, { zero, zero });
}

llvm::Value * Generator::visit(BinaryOp & node)
{
    auto left = node.left->accept(this);
    auto right = node.right->accept(this);

    // get the type coercion.
    auto coercion = typeSys.coerce(left->getType(), right->getType());

    // cast the two values to the coerce type (note: if one of them is already of that type,
    // then no cast is made).
    auto leftCast = typeSys.cast(left, coercion, builder.GetInsertBlock());
    auto rightCast = typeSys.cast(right, coercion, builder.GetInsertBlock());

    // check if this is a math of a comparison operator.
    if (node.op == "+" || node.op == "-" || node.op == "/" || node.op == "*" || node.op == "%") {
        return llvm::BinaryOperator::Create(typeSys.getMathOp(coercion, node.op),
                leftCast, rightCast, "", builder.GetInsertBlock());
    } else {
        if (typeSys.isFP(coercion)) {
            return builder.CreateFCmp(typeSys.getCmpPredicate(coercion, node.op), leftCast, rightCast);
        } else {
            return builder.CreateICmp(typeSys.getCmpPredicate(coercion, node.op), leftCast, rightCast);
        }
    }
}

llvm::Value * Generator::visit(If & node)
{
    auto condition = node.condition->accept(this);

    auto function = builder.GetInsertBlock()->getParent();

    auto thenBlock = llvm::BasicBlock::Create(*context, "then", function);
    auto elseBlock = llvm::BasicBlock::Create(*context, "else");
    auto mergeBlock = llvm::BasicBlock::Create(*context, "ifcont");

    if (node.elseBlock) {
        builder.CreateCondBr(condition, thenBlock, elseBlock);
    } else {
        builder.CreateCondBr(condition, thenBlock, mergeBlock);
    }

    builder.SetInsertPoint(thenBlock);

    createScope();
    node.thenBlock->accept(this);
    popScope();

    thenBlock = builder.GetInsertBlock();

    if (thenBlock->getTerminator() == nullptr) {
        builder.CreateBr(mergeBlock);
    }

    if (node.elseBlock) {
        function->getBasicBlockList().push_back(elseBlock);
        builder.SetInsertPoint(elseBlock);

        createScope();
        node.elseBlock->accept(this);
        popScope();

        builder.CreateBr(mergeBlock);
    }

    function->getBasicBlockList().push_back(mergeBlock);
    builder.SetInsertPoint(mergeBlock);

    return nullptr;
}

llvm::Value * Generator::visit(Print & node)
{
    // take the value we want to print.
    auto value = node.expression->accept(this);

    auto print = module->getOrInsertFunction("printf",
            llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*context),
                    llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
                    true)
    );

    // create the format type accordingly.
    std::string fType;

    if (value->getType()->isIntegerTy()) {
        fType = "%d";
    } else if (value->getType()->isFloatingPointTy()) {
        fType = "%g";
    } else if (value->getType()->isPointerTy()) {
        fType = "%s";
    }

    auto format = String(fType + "\n").accept(this);

    // if this is a floating pointer, make sure it's a double, since printf needs doubles.
    if (value->getType()->isFloatingPointTy()) {
        value = typeSys.cast(value, typeSys.doubleTy, builder.GetInsertBlock());
    }

    return builder.CreateCall(print, { format, value });
}

llvm::Value * Generator::visit(For & node)
{
    VarDecl initial(node.variable, nullptr, node.initial);
    auto counter = initial.accept(this);

    // create the block.
    auto function = builder.GetInsertBlock()->getParent();
    auto block = llvm::BasicBlock::Create(*context, "forloop", function);
    auto after = llvm::BasicBlock::Create(*context, "forcont");
    auto condition = node.condition->accept(this);

    // fall to the block.
    builder.CreateCondBr(condition, block, after);

    builder.SetInsertPoint(block);

    createScope();
    node.block->accept(this);
    popScope();

    // increment the counter.
    auto variable = builder.CreateLoad(counter);
    auto result = builder.CreateAdd(variable, node.increment->accept(this), "counter");
    builder.CreateStore(result, counter);

    // execute again or stop.
    condition = node.condition->accept(this);
    builder.CreateCondBr(condition, block, after);

    // insert the after block.
    function->getBasicBlockList().push_back(after);
    builder.SetInsertPoint(after);

    return nullptr;
}

llvm::Value * Generator::visit(Array & node)
{
    auto first = node.elements[0]->accept(this);
    auto size = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), node.elements.size(), true);
    auto array = builder.CreateAlloca(first->getType(), size);

    uint i = 0;
    for (auto e : node.elements) {
        auto value = e->accept(this);
        auto index = llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context), i, true);
        auto ptr = llvm::GetElementPtrInst::CreateInBounds(array, {index}, "", builder.GetInsertBlock());
        builder.CreateStore(value, ptr);
        i++;
    }

    return array;
}

llvm::Value * Generator::visit(ArrayIndex & node)
{
    auto array = scope().get(node.name);

    auto index = node.expression->accept(this);
    auto loadedArray = builder.CreateLoad(array);
    auto ptr = llvm::GetElementPtrInst::CreateInBounds(loadedArray, {index}, "", builder.GetInsertBlock());

    return builder.CreateLoad(ptr);
}

llvm::Value * Generator::visit(Bool & node)
{
    return llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(llvm::getGlobalContext()),
            (uint64_t) node.boolean, false);
}

llvm::Value * Generator::visit(Struct & node)
{
    std::vector<llvm::Type*> memberTypes;
    std::vector<std::string> members;

    auto structType = llvm::StructType::create(*context, node.name);
    typeSys.addStructType(node.name, structType);

    for (auto m : node.members) {
        members.push_back(m->name);
        memberTypes.push_back(m->type->getType(typeSys));
    }

    if (structType->isOpaque()) {
        structType->setBody(memberTypes, false);
    }

    typeSys.setStructMembers(node.name, members);

    return nullptr;
}

llvm::Value * Generator::visit(StructAssignment & node)
{
    auto structPtr = scope().get(node.variable);
    auto load = builder.CreateLoad(structPtr);

    auto underlyingStruct = new llvm::LoadInst(load);

    if (!underlyingStruct->getType()->isStructTy()) {
        throw std::runtime_error("This is not a struct: " + node.variable);
    }

    std::string name = underlyingStruct->getType()->getStructName().str();
    int memberIndex = typeSys.getStructMemberIndex(name, node.member);

    auto value = node.expression->accept(this);
    auto zero = llvm::ConstantInt::get(typeSys.intTy, 0, true);
    auto index = llvm::ConstantInt::get(typeSys.intTy, (uint64_t) memberIndex, true);
    auto ptr = builder.CreateInBoundsGEP(load, { zero, index });

    return builder.CreateStore(value, ptr);
}

llvm::Value * Generator::visit(StructMember & node)
{
    auto structPtr = scope().get(node.variable);
    auto load = builder.CreateLoad(structPtr);

    auto underlyingStruct = new llvm::LoadInst(load);

    if (!underlyingStruct->getType()->isStructTy()) {
        throw std::runtime_error("This is not a struct: " + node.variable);
    }

    std::string name = underlyingStruct->getType()->getStructName().str();
    int memberIndex = typeSys.getStructMemberIndex(name, node.member);

    auto zero = llvm::ConstantInt::get(typeSys.intTy, 0, true);
    auto index = llvm::ConstantInt::get(typeSys.intTy, (uint64_t) memberIndex, true);
    auto ptr = builder.CreateInBoundsGEP(load, { zero, index });

    return builder.CreateLoad(ptr);
}
