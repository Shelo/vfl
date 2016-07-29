#include <iostream>
#include <vector>
#include <fstream>

#include "vfs/ast/SyntaxTree.hpp"
#include "vfs/ast/Generator.hpp"

extern int yyparse();

std::vector<std::shared_ptr<Function>> program;

int main(int argc, char *argv[])
{
	Generator generator;

    // redirect input.
    freopen(argv[1], "r", stdin);

    try {
        yyparse();

        try {
            generator.generate(program);
            generator.dump();
        } catch (const std::exception & e) {
            std::cerr << "Generation error:" << std::endl;
            std::cerr << "\033[1;31m" << e.what() << "\033[0m" << std::endl << std::endl;
        }

    } catch (const std::exception & e) {
        std::cerr << "Syntax error:" << std::endl;
        std::cerr << "\033[1;31m" << e.what() << "\033[0m" << std::endl;
    }

    return 0;
}
