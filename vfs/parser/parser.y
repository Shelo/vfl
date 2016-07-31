%{
	#include <iostream>
	#include <string>
	#include <vector>
	#include <memory>

	#include "../type/Types.hpp"
	#include "../ast/SyntaxTree.hpp"

	size_t yyline = 1;

	int yydebug = 1;

	extern int yylex();

	extern std::vector<std::shared_ptr<Function>> program;
	extern std::vector<std::shared_ptr<Struct>> structs;

	void yyerror(const char *str)
	{
        throw std::runtime_error(std::string(str) + " at line " + std::to_string(yyline));
	}
%}

%union
{
	std::vector<std::shared_ptr<Parameter>> * parameterList;
	std::vector<std::shared_ptr<Expression>> * expressionList;

	Block * block;
	Function * function;
	Struct * structDef;
	Statement * statement;
	Expression * expression;
	Type * type;

	std::string * string;
	int integer;
	int token;
	float floatNumber;
}

%error-verbose

%token VAR ASSIGN END RETURN IF ELSE PRINT VOID FOR TRUE FALSE PRINT_F

%token <token> PLUS MINUS MULT DIV EQ NEQ LESS GREATER LEQ GEQ MOD
%token <integer> INTEGER
%token <floatNumber> FLOAT
%token <string> FUNCTION_NAME STRUCT_NAME IDENTIFIER STRING

%type <structDef> struct
%type <function> function
%type <parameterList> parameterList structMembers
%type <block> block
%type <type> typeName parameterName
%type <statement> statement assignment return variableDeclaration if print for
%type <expression> expression versionInv functionCall
%type <expressionList> expressionList

%left EQ NEQ
%left LESS GREATER
%left LEQ GEQ
%left PLUS MINUS
%left MULT DIV MOD

%right IDENTIFIER '['

%start program

%%

program:
	// empty
	| program function
	{
		program.push_back(std::shared_ptr<Function>($2));
	}
	| program struct
    {
        structs.push_back(std::shared_ptr<Struct>($2));
    }
	;

function:
	FUNCTION_NAME '(' parameterList ')' block END
	{
		$$ = new Function(*$1, "", *$3, std::shared_ptr<Block>($5));
	}
	| FUNCTION_NAME '(' parameterList ')' ':' typeName block END
	{
		$$ = new Function(*$1, "", *$3, std::shared_ptr<Type>($6), std::shared_ptr<Block>($7));
	}
	| FUNCTION_NAME '(' IDENTIFIER ';' parameterList ')' block END
	{
		$$ = new Function(*$1, *$3, *$5, std::shared_ptr<Block>($7));
	}
	| FUNCTION_NAME '(' IDENTIFIER ';' parameterList ')' ':' typeName block END
	{
		$$ = new Function(*$1, *$3, *$5, std::shared_ptr<Type>($8), std::shared_ptr<Block>($9));
	}
	;

parameterList:
	// empty
	{
		$$ = new std::vector<std::shared_ptr<Parameter>>();
	}
	| parameterList ',' parameterName IDENTIFIER
	{
		$1->push_back(std::make_shared<Parameter>(*$4, std::shared_ptr<Type>($3)));
	}
	| parameterName IDENTIFIER
	{
		$$ = new std::vector<std::shared_ptr<Parameter>>();
		$$->push_back(std::make_shared<Parameter>(*$2, std::shared_ptr<Type>($1)));
	}
	;

struct:
    STRUCT_NAME structMembers END
    {
        $$ = new Struct(*$1, *$2);
    }
    ;

structMembers:
    // empty.
    {
        $$ = new std::vector<std::shared_ptr<Parameter>>();
    }
    | structMembers IDENTIFIER IDENTIFIER
    {
        $1->push_back(std::make_shared<Parameter>(*$3, std::make_shared<Type>(*$2)));
    }
    | structMembers STRUCT_NAME IDENTIFIER
    {
        $1->push_back(std::make_shared<Parameter>(*$3, std::make_shared<StructType>(*$2)));
    }
    ;


variableDeclaration:
	VAR IDENTIFIER ':' typeName ASSIGN expression
	{
		$$ = new VarDecl(*$2, std::shared_ptr<Type>($4), std::shared_ptr<Expression>($6));
	}
	| VAR IDENTIFIER ASSIGN expression
	{
		$$ = new VarDecl(*$2, std::shared_ptr<Type>(nullptr), std::shared_ptr<Expression>($4));
	}
	| VAR IDENTIFIER ':' typeName
	{
		$$ = new VarDecl(*$2, std::shared_ptr<Type>($4));
	}
	;

typeName:
	IDENTIFIER
	{
		$$ = new Type(*$1);
	}
	| IDENTIFIER '[' expression ']'
	{
		$$ = new ArrayType(*$1, std::shared_ptr<Expression>($3));
	}
	| STRUCT_NAME
	{
	    $$ = new StructType(*$1);
	}
	;

parameterName:
	typeName
	| IDENTIFIER '[' ']'
	{
		$$ = new ArrayType(*$1);
	}
	;

block:
	// empty
	{
		$$ = new Block();
	}
	| block statement
	{
		$1->statements.push_back(std::shared_ptr<Statement>($2));
	}
	;

statement:
	assignment
	| expression
	{
		$$ = new ExpressionStatement(std::shared_ptr<Expression>($1));
	}
	| return
	| variableDeclaration
	| if
	| print
	| for
	;

assignment:
	IDENTIFIER ASSIGN expression
	{
		$$ = new Assignment(*$1, std::shared_ptr<Expression>($3));
	}
	| IDENTIFIER '[' expression ']' ASSIGN expression
	{
		$$ = new ArrayAssignment(*$1, std::shared_ptr<Expression>($3), std::shared_ptr<Expression>($6));
	}
	| IDENTIFIER '.' IDENTIFIER ASSIGN expression
	{
		$$ = new StructAssignment(*$1, *$3, std::shared_ptr<Expression>($5));
	}
	;

expression:
	functionCall
	| versionInv
	| STRING
	{
		$$ = new String(*$1);
	}
	| IDENTIFIER '[' expression ']'
	{
		$$ = new ArrayIndex(*$1, std::shared_ptr<Expression>($3));
	}
	| IDENTIFIER '.' IDENTIFIER
	{
		$$ = new StructMember(*$1, *$3);
	}
	| IDENTIFIER
	{
		$$ = new Identifier(*$1);
	}
	| INTEGER
	{
		$$ = new Integer($1);
	}
	| FLOAT
	{
		$$ = new Float($1);
	}
	| expression PLUS expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "+", std::shared_ptr<Expression>($3));
	}
	| expression MULT expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "*", std::shared_ptr<Expression>($3));
	}
	| expression DIV expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "/", std::shared_ptr<Expression>($3));
	}
	| expression MINUS expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "-", std::shared_ptr<Expression>($3));
	}
	| expression EQ expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "==", std::shared_ptr<Expression>($3));
	}
	| expression NEQ expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "!=", std::shared_ptr<Expression>($3));
	}
	| expression LESS expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "<", std::shared_ptr<Expression>($3));
	}
	| expression GREATER expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), ">", std::shared_ptr<Expression>($3));
	}
	| expression LEQ expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "<=", std::shared_ptr<Expression>($3));
	}
	| expression GEQ expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), ">=", std::shared_ptr<Expression>($3));
	}
	| expression MOD expression
	{
		$$ = new BinaryOp(std::shared_ptr<Expression>($1), "%", std::shared_ptr<Expression>($3));
	}
	| '(' expression ')'
	{
		$$ = $2;
	}
	| '[' expressionList ']'
	{
		$$ = new Array(*$2);
	}
	| TRUE
	{
	    $$ = new Bool(true);
	}
	| FALSE
	{
	    $$ = new Bool(false);
	}
	;

functionCall:
	FUNCTION_NAME '(' expressionList ')'
	{
		$$ = new FunctionCall(*$1, "", *$3);
	}
	| FUNCTION_NAME '(' IDENTIFIER ';' expressionList ')'
	{
		$$ = new FunctionCall(*$1, *$3, *$5);
	}

versionInv:
	'@' '(' expressionList ')'
	{
		$$ = new VersionInv("", *$3);
	}
	| '@' '(' IDENTIFIER ';' expressionList ')'
	{
		$$ = new VersionInv(*$3, *$5);
	}
	;

expressionList:
	// empty
	{
		$$ = new std::vector<std::shared_ptr<Expression>>();
	}
	| expressionList ',' expression
	{
		$1->push_back(std::shared_ptr<Expression>($3));
	}
	| expression
	{
		$$ = new std::vector<std::shared_ptr<Expression>>();
		$$->push_back(std::shared_ptr<Expression>($1));
	}
	;

return:
	RETURN VOID
	{
		$$ = new Return();
	}
	| RETURN expression
	{
		$$ = new Return(std::shared_ptr<Expression>($2));
	}
	;

if:
	IF expression '{' block '}'
	{
		$$ = new If(std::shared_ptr<Expression>($2), std::shared_ptr<Block>($4), std::shared_ptr<Block>(nullptr));
	}
	| IF expression '{' block '}' ELSE '{' block '}'
	{
		$$ = new If(std::shared_ptr<Expression>($2), std::shared_ptr<Block>($4), std::shared_ptr<Block>($8));
	}
	;

print:
	PRINT expression
	{
		$$ = new Print(std::shared_ptr<Expression>($2));
	}
	;

for:
	FOR IDENTIFIER ASSIGN expression ',' expression '{' block '}'
	{
		$$ = new For(*$2, std::shared_ptr<Expression>($4), std::shared_ptr<Expression>($6),
				std::shared_ptr<Block>($8));
	}
	| FOR IDENTIFIER ASSIGN expression ',' expression ',' expression '{' block '}'
	{
		$$ = new For(*$2, std::shared_ptr<Expression>($4), std::shared_ptr<Expression>($6),
				std::shared_ptr<Block>($10), std::shared_ptr<Expression>($8));
	}
	;
