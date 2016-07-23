%{
	#include <string>
	#include "../ast/SyntaxTree.hpp"
	#include "parser.hpp"

	#define TOKEN(T) { return T; }
	#define SAVE_TOKEN(T) { yylval.token = T; return T; }
	
	extern size_t lineNumber;
%}

%option noyywrap

%x S_COMMENT
%x S_STRING
%%

void 					TOKEN(VOID);
print					TOKEN(PRINT);
var                    	TOKEN(VAR);
return                 	TOKEN(RETURN);
if                     	TOKEN(IF);
else                   	TOKEN(ELSE);
@End                   	TOKEN(END);
@[A-Z][a-zA-Z0-9]*    	yylval.string = new std::string(yytext + 1, yyleng - 1); TOKEN(FUNCTION_NAME);
[a-z][a-zA-Z0-9]*      	yylval.string = new std::string(yytext, yyleng); TOKEN(IDENTIFIER);
[0-9]+\.[0-9]*f        	yylval.floatNumber = std::stof(yytext); TOKEN(FLOAT);
[0-9]+                 	yylval.integer = std::stoi(yytext); TOKEN(INTEGER);
==                     	TOKEN(EQ);
!=                     	TOKEN(NEQ);
\<						TOKEN(LESS);
\>						TOKEN(GREATER);
\<=						TOKEN(LEQ);
\>=						TOKEN(GEQ);
\%						TOKEN(MOD);
=                      	TOKEN(ASSIGN);
\/\/                   	BEGIN(S_COMMENT);
\"                     	BEGIN(S_STRING);
\n                     	/* ignore EOL */ yyline++;
[ \t]+                 	/* ignore whitespace */;
\+                     	SAVE_TOKEN(PLUS);
\-                     	SAVE_TOKEN(MINUS);
\*                     	SAVE_TOKEN(MULT);
\/                     	SAVE_TOKEN(DIV);
.                      	return (int) yytext[0];

<S_COMMENT>[^\n]*       BEGIN(INITIAL);

<S_STRING>[^\"]*        yylval.string = new std::string(yytext); TOKEN(STRING);
<S_STRING>\"            BEGIN(INITIAL);

%%
