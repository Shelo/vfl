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
	lastFunction = function;
	
	// create the block for this function.
	lastBlock = llvm::BasicBlock::Create(*context, "entry", function);
	builder.SetInsertPoint(lastBlock);
	
	// create the scope.
	createScope();

	int i = 0;
	for (auto & arg : function->args()) {
		auto parameter = node.parameters[i];
		arg.setName(parameter->name);

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
	return builder.CreateAlloca(parameter.type->getType(), nullptr, parameter.name);
}

llvm::Value * Generator::visit(Block & node)
{
	llvm::Value * last = nullptr;
	for (auto i : node.statements) {
		last = i->accept(this);
	}
	
	return last;
}

llvm::Value * Generator::visit(VarDecl & node)
{
	llvm::Value * initial = nullptr;
	if (node.expression != nullptr) {
		initial = node.expression->accept(this);
	}
	
	llvm::Type * type;
	if (node.type == nullptr) {
		if (initial == nullptr) {
			throw "Variable type inference needs a definition.";
		}
		
		type = initial->getType();
	} else {
		type = node.type->getType();
	}

	// allocate memory on the stack.
	auto value = builder.CreateAlloca(type, nullptr, node.name);
	scope().add(node.name, value);

	if (initial != nullptr) {
		builder.CreateStore(initial, value);
	}

	return value;
}

llvm::Value * Generator::visit(Assignment & node)
{
	auto value = scope().get(node.variable);	
	return builder.CreateStore(node.expression->accept(this), value);
}

llvm::Value * Generator::visit(VersionInv & node)
{	
	auto name = (std::string) lastFunction->getName();
	auto virtualName = node.getVirtualName(name);
	auto function = module->getFunction(virtualName);

	if (function == nullptr) {
		throw "Function not defined: " + name;
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
		throw "Function not defined: " + node.name;
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
		
		if (returnValue->getType()->isVoidTy()) {
			returnValue = nullptr;
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
	return builder.CreateLoad(scope().get(node.name));
}

llvm::Value * Generator::visit(Integer & node)
{	
	return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context), node.value, true);
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

	llvm::Instruction::BinaryOps instruction;
	
	if (node.op == "+") {
		instruction = llvm::Instruction::Add; 
	} else if (node.op == "-") {
		instruction = llvm::Instruction::Sub;
	} else if (node.op == "*") {
		instruction = llvm::Instruction::Mul;
	} else if (node.op == "/") {
		instruction = llvm::Instruction::SDiv;
	} else if (node.op == "%") {
		return builder.CreateSRem(left, right);
	} else if (node.op == "==") {
		return builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ, left, right);
	} else if (node.op == "!=") {
		return builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_NE, left, right);
	} else if (node.op == "<") {
		return builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLT, left, right);
	} else if (node.op == ">") {
		return builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SGT, left, right);
	} else if (node.op == "<=") {
		return builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SLE, left, right);
	} else if (node.op == ">=") {
		return builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_SGE, left, right);
	} else {
		throw "Unknown binary operator: " + node.op;
	}

	return llvm::BinaryOperator::Create(instruction, left, right, "", builder.GetInsertBlock());
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

	auto format = llvm::ConstantDataArray::getString(*context, "%d\n");
	auto formatVar = new llvm::GlobalVariable(
		*module, llvm::ArrayType::get(llvm::IntegerType::get(*context, 8), 4),
		true, llvm::GlobalValue::PrivateLinkage, format, ".str");

	auto zero = llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(*context));

	// these are to reference the array, first zero is for offset from the pointer, the second
	// zero is for offset in the elements.
	std::vector<llvm::Constant*> indices;
	indices.push_back(zero);
	indices.push_back(zero);

	auto ptr = llvm::ConstantExpr::getGetElementPtr(formatVar, indices);
	
	std::vector<llvm::Value*> arguments;
	arguments.push_back(ptr);
	arguments.push_back(node.expression->accept(this));

	return builder.CreateCall(print, arguments);
}
