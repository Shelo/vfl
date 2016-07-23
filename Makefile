all: vfsc

vfs/gen/Parser.cpp:
	bison -d -o $@ vfs/parser/parser.y

vfs/gen/Lexer.cpp: vfs/gen/Parser.cpp
	lex -o $@ vfs/parser/lexer.l

clean:
	rm -f builder/vfsc
	rm -f vfs/gen/*

vfsc: clean vfs/gen/Lexer.cpp
	g++ -o build/vfsc main.cpp vfs/ast/*.cpp vfs/gen/*.cpp \
		`/usr/local/opt/llvm/bin/llvm-config --libs all native --cxxflags --ldflags --system-libs` \
		-I/usr/local/opt/llvm/include -L/usr/local/opt/llvm/lib --std=c++11 -fexceptions \
		-Wno-unused-function -Wno-reorder -Wno-redundant-move -Wno-non-virtual-dtor -Wno-deprecated-register

run: vfsc
	build/vfsc < tests/simple.vfs

test:
	build/vfsc < tests/simple.vfs
