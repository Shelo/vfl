#include "Generator.hpp"


void Generator::generate(std::vector<std::shared_ptr<Function>> program)
{
	for (auto f : program) {
		f->accept(this);
	}
}

llvm::Value * Generator::visit(Function & node)
{
	std::vector<llvm::Type*> parameterTypes;
	for (auto i : node.parameters) {
		parameterTypes.push_back(i->type->getType());
	}

	std::string name = node.name;
	if (node.name == "Main") {
		name = "main";
	} else {
		name = node.getVirtualName();
	}

	auto type = llvm::FunctionType::get(node.type->getType(), llvm::makeArrayRef(parameterTypes), false);
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

		if (value != nullptr) {
			builder.CreateStore(&arg, value);
		} else {
			value = &arg;
		}

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
	if (parameter.type->isArray()) {
		return nullptr;
	}

	return builder.CreateAlloca(parameter.type->getType(), nullptr, parameter.name);
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

	bool isArray = false;

	llvm::Type * type = nullptr;
	if (node.type == nullptr) {
		if (initial == nullptr) {
			throw std::runtime_error("Variable type inference needs a definition.");
		}

		type = initial->getType();
	} else {
		type = node.type->getType();
		isArray = node.type->isArray();
	}

	// NOTE: for some reason, in my version of LLVM, ArrayTyID is 13, but
	// for an array the id is actually 14.
	// TODO: HERE, IF THIS AN ARRAY OR STRUCT, WE WILL HAVE TO COPY IT.
	if ((type->getTypeID() == 14) && initial != nullptr) {
		initial->setName(node.name);
		value = initial;
	} else if (type->getTypeID() == 14 || isArray) {
		auto arrayType = reinterpret_cast<ArrayType*>(node.type.get());
		auto size = arrayType->size->accept(this);
		value = builder.CreateAlloca(type, size, node.name);
		// TODO: fill the array with default values.
	} else {
		value = builder.CreateAlloca(type, nullptr, node.name);

		if (initial != nullptr) {
			builder.CreateStore(initial, value);
		}
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

	auto value = node.expression->accept(this);
	auto index = node.index->accept(this);
	auto ptr = llvm::GetElementPtrInst::CreateInBounds(array, { index }, "", builder.GetInsertBlock());

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

	std::vector<llvm::Value*> values;
	for (auto i : node.arguments) {
		values.push_back(i->accept(this));
	}

	return builder.CreateCall(function, values);
}

llvm::Value * Generator::visit(FunctionCall & node)
{
	auto function = module->getFunction(node.getVirtualName());

	if (function == nullptr) {
		throw std::runtime_error("Function not defined: " + node.name);
	}

	// TODO: check arg compatibility.

	std::vector<llvm::Value*> values;
	for (auto i : node.arguments) {
		values.push_back(i->accept(this));
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

	if (llvm::isa<llvm::AllocaInst>(value)) {
		auto alloc = llvm::dyn_cast<llvm::AllocaInst>(value);

		if (alloc->isArrayAllocation()) {
			return value;
		}
	}

	auto name = value->getName().str();
	if (name.find("param.") == 0) {
		return value;
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
	return nullptr;
}

llvm::Value * Generator::visit(BinaryOp & node)
{
	auto left = node.left->accept(this);
	auto right = node.right->accept(this);

    auto coercion = typeSys.coerce(left->getType(), right->getType());

    auto leftCast = typeSys.cast(left, coercion, builder.GetInsertBlock());
    auto rightCast = typeSys.cast(right, coercion, builder.GetInsertBlock());

    // check if this is a math of a compare operator.
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
	node.thenBlock->accept(this);
	thenBlock = builder.GetInsertBlock();

	if (thenBlock->getTerminator() == nullptr) {
		builder.CreateBr(mergeBlock);
	}

	if (node.elseBlock) {
		function->getBasicBlockList().push_back(elseBlock);
		builder.SetInsertPoint(elseBlock);
		node.elseBlock->accept(this);
		builder.CreateBr(mergeBlock);
	}

	function->getBasicBlockList().push_back(mergeBlock);
	builder.SetInsertPoint(mergeBlock);

	return nullptr;
}

llvm::Value * Generator::visit(Print & node)
{
	auto print = module->getOrInsertFunction("printf",
			llvm::FunctionType::get(llvm::IntegerType::getInt32Ty(*context),
			llvm::PointerType::get(llvm::Type::getInt8Ty(*context), 0),
			true)
	);

	auto format = llvm::ConstantDataArray::getString(*context, "%g\n");
	auto formatVar = new llvm::GlobalVariable(
		*module, llvm::ArrayType::get(llvm::IntegerType::get(*context, 8), 4),
		true, llvm::GlobalValue::PrivateLinkage, format, ".str");

	auto zero = llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(*context));

	// these are to reference the array, first zero is for offset from the pointer, the second
	// zero is for offset in the elements.
	auto ptr = llvm::GetElementPtrInst::CreateInBounds(formatVar, { zero, zero }, "", builder.GetInsertBlock());

    // take the value and cast to a double.
    auto value = node.expression->accept(this);
    value = typeSys.cast(value, llvm::Type::getDoubleTy(llvm::getGlobalContext()), builder.GetInsertBlock());

	std::vector<llvm::Value*> arguments;
	arguments.push_back(ptr);
	arguments.push_back(value);

	return builder.CreateCall(print, arguments);
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
	node.block->accept(this);

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
		auto ptr = llvm::GetElementPtrInst::CreateInBounds(array, { index }, "", builder.GetInsertBlock());
		builder.CreateStore(value, ptr);
		i++;
	}

	return array;
}

llvm::Value * Generator::visit(ArrayIndex & node)
{
	auto array = scope().get(node.name);

	auto index = node.expression->accept(this);
	auto ptr = llvm::GetElementPtrInst::CreateInBounds(array, { index }, "", builder.GetInsertBlock());

	return builder.CreateLoad(ptr);
}
