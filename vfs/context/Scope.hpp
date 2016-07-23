#pragma once

#include <llvm/IR/Value.h>

#include <map>

class Scope
{
private:
	std::map<std::string, llvm::Value*> table;

public:
	void add(std::string name, llvm::Value * value)
	{
		if (table.find(name) != table.end()) {
			throw "Symbol already declared: " + name;
		}
		
		table[name] = value;
	}
	
	llvm::Value * get(std::string name)
	{
		auto it = table.find(name);
		
		if (it != table.end()) {
			return it->second;
		}
		
		return nullptr;
	}
};
