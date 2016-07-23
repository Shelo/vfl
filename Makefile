all: vfsc

vfs/gen/Parser.cpp:
	bison -d -o $@ vfs/parser/parser.y

vfs/gen/Lexer.cpp: vfs/gen/Parser.cpp
	lex -o $@ vfs/parser/lexer.l

clean:
	rm -f vfsc
	rm -f vfs/gen/*

vfsc: clean vfs/gen/Lexer.cpp
	g++ -o build/vfsc main.cpp vfs/ast/*.cpp vfs/gen/*.cpp `llvm-config --libs core native --cxxflags --ldflags` --std=c++11 -Wno-unused-function -Wno-reorder -fexceptions

run: vfsc
	build/vfsc < tests/simple.vfs

test:
	build/vfsc < tests/simple.vfs
