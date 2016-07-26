#include <iostream>
#include <vector>

#include "vfs/ast/SyntaxTree.hpp"
#include "vfs/ast/Generator.hpp"

extern int yyparse();

std::vector<std::shared_ptr<Function>> program;

int main(int argc, char *argv[])
{
	Generator generator;

	yyparse();

	generator.generate(program);
	generator.dump();
}
