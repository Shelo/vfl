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

	std::string * string;
	int integer;
	int token;
	float floatNumber;
}

%token VAR ASSIGN END RETURN IF ELSE

%token <token> PLUS MINUS MULT DIV EQ NEQ LESS GREATER LEQ GEQ
%token <integer> INTEGER
%token <floatNumber> FLOAT
%token <string> FUNCTION_NAME
%token <string> IDENTIFIER
%token <string> STRING

%type <function> function
%type <parameterList> parameterList
%type <block> block
%type <statement> statement assignment return variableDeclaration if
%type <expression> expression versionInv functionCall
%type <expressionList> expressionList

%left EQUALS NEQ
%left LESS GREATER LEQ GEQ
%left PLUS MINUS
%left MULT DIV

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
	| FUNCTION_NAME '(' parameterList ')' ':' IDENTIFIER block END
	{
		$$ = new Function(*$1, "", *$3, Type::create(*$6), std::shared_ptr<Block>($7));
	}
	| FUNCTION_NAME '(' IDENTIFIER ';' parameterList ')' block END
	{
		$$ = new Function(*$1, *$3, *$5, std::shared_ptr<Block>($7));
	}
	| FUNCTION_NAME '(' IDENTIFIER ';' parameterList ')' ':' IDENTIFIER block END
	{
		$$ = new Function(*$1, *$3, *$5, Type::create(*$8), std::shared_ptr<Block>($9));
	}
	;

parameterList:
	// empty
	{
		$$ = new std::vector<std::shared_ptr<Parameter>>();
	}
	| parameterList ',' IDENTIFIER IDENTIFIER
	{
		$1->push_back(std::make_shared<Parameter>(*$4, Type::create(*$3)));
	}
	| IDENTIFIER IDENTIFIER
	{
		$$ = new std::vector<std::shared_ptr<Parameter>>();
		$$->push_back(std::make_shared<Parameter>(*$2, Type::create(*$1)));
	}
	;

variableDeclaration:
	VAR IDENTIFIER ':' IDENTIFIER ASSIGN expression
	{
		$$ = new VarDecl(*$2, Type::create(*$4), std::shared_ptr<Expression>($6));
	}
	| VAR IDENTIFIER ASSIGN expression
	{
		$$ = new VarDecl(*$2, std::shared_ptr<Type>(nullptr), std::shared_ptr<Expression>($4));
	}
	| VAR IDENTIFIER ':' IDENTIFIER
	{
		$$ = new VarDecl(*$2, Type::create(*$4));
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
	;

assignment:
	IDENTIFIER ASSIGN expression
	{
		$$ = new Assignment(*$1, std::shared_ptr<Expression>($3));
	}
	;

expression:
	functionCall
	| versionInv
	| STRING
	{
		$$ = new String(*$1);
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
	| '(' expression ')'
	{
		$$ = $2;
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
	RETURN expression
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