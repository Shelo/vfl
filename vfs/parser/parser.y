%{
	#include <iostream>
	#include <string>
	#include <vector>
	#include <memory>

	#include "../ast/SyntaxTree.hpp"

	size_t yyline = 1;

	int yydebug = 1;

	extern int yylex();
	extern std::vector<std::shared_ptr<Function>> program;

	void yyerror(const char *str)
	{
		std::cerr << "error: " << str << " at line " << yyline << std::endl;
	}
%}

%union
{
	std::vector<std::shared_ptr<Parameter>> * parameterList;
	std::vector<std::shared_ptr<Expression>> * expressionList;

	Block * block;
	Function * function;
	Statement * statement;
	Expression * expression;
	Type * type;

	std::string * string;
	int integer;
	int token;
	float floatNumber;
}

%token VAR ASSIGN END RETURN IF ELSE PRINT VOID

%token <token> PLUS MINUS MULT DIV EQ NEQ LESS GREATER LEQ GEQ MOD
%token <integer> INTEGER
%token <floatNumber> FLOAT
%token <string> FUNCTION_NAME
%token <string> IDENTIFIER
%token <string> STRING

%type <function> function
%type <parameterList> parameterList
%type <block> block
%type <type> typeName
%type <statement> statement assignment return variableDeclaration if print
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
	| parameterList ',' typeName IDENTIFIER
	{
		$1->push_back(std::make_shared<Parameter>(*$4, std::shared_ptr<Type>($3)));
	}
	| typeName IDENTIFIER
	{
		$$ = new std::vector<std::shared_ptr<Parameter>>();
		$$->push_back(std::make_shared<Parameter>(*$2, std::shared_ptr<Type>($1)));
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
	| IDENTIFIER '[' INTEGER ']'
	{
		$$ = new ArrayType(*$1, $3);
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
	