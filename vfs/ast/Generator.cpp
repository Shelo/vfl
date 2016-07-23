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
	}

	auto virtualName = node.getVirtualName();

	auto type = llvm::FunctionType::get(node.type->getType(), llvm::makeArrayRef(parameterTypes), false);
	auto function = llvm::Function::Create(type, llvm::Function::ExternalLinkage, virtualName, module.get());
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
	
	popScope();

	return function;
}

llvm::Value * Generator::visit(Parameter & parameter)
{
	return builder.CreateAlloca(parameter.type->getType(), nullptr, parameter.name);
}

llvm::Value * Generator::visit(Block & node)
{
	for (auto i : node.statements) {
		i->accept(this);
	}
	
	return nullptr;
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
	return builder.CreateRet(node.expression->accept(this));
}

llvm::Value * Generator::visit(ExpressionStatement & node)
{	
	return node.expression->accept(this);
}

llvm::Value * Generator::visit(Identifier & node)
{	
	auto value = scope().get(node.name);
	return builder.CreateLoad(value);
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

llvm::Instruction::BinaryOps getInstructionForOperator(std::string op)
{
	if (op == "+") {
		return llvm::Instruction::Add; 
	} else if (op == "-") {
		return llvm::Instruction::Sub;
	} else if (op == "*") {
		return llvm::Instruction::Mul;
	} else if (op == "/") {
		return llvm::Instruction::SDiv;
	}
	
	throw "Unkown operator: " + op;	
}

llvm::Value * Generator::visit(BinaryOp & node)
{	
	auto left = node.left->accept(this);
	auto right = node.right->accept(this);

	auto instruction = getInstructionForOperator(node.op);

	return llvm::BinaryOperator::Create(instruction, left, right, "", builder.GetInsertBlock());
}

llvm::Value * Generator::visit(If & node)
{	
	return nullptr;
}
