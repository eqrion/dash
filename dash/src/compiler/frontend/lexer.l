%{
	#include "../ast.h"
	#include "parser.h"
	
	#define YY_USER_ACTION { \
			yylloc->first_line = yylineno; \
	 		yylloc->last_line = yylineno; \
	 		yylloc->first_column = yycolumn; \
	 		yylloc->last_column = yycolumn + yyleng; }
%}

%option outfile="lexer.c"
%option warn nodefault nounistd never-interactive

%option reentrant noyywrap yylineno
%option bison-bridge bison-locations

%%

"def"		{ return TOKEN_DEF; }
"->"		{ return TOKEN_ARROW; }
"let"		{ return TOKEN_LET; }
"while"		{ return TOKEN_WHILE; }
"if"		{ return TOKEN_IF; }
"else"		{ return TOKEN_ELSE; }
"return"	{ return TOKEN_RETURN; }

"+"			{ return TOKEN_OP_ADD; }
"-"			{ return TOKEN_OP_SUB; }
"*"			{ return TOKEN_OP_MUL; }
"/"			{ return TOKEN_OP_DIV; }

"<"			{ return TOKEN_OP_LESS; }
"<="		{ return TOKEN_OP_LESS_EQ; }
">"			{ return TOKEN_OP_GREATER; }
">="		{ return TOKEN_OP_GREATER_EQ; }

"real"		{ yylval->type = dst_type_real; return TOKEN_TYPE; }
"integer"	{ yylval->type = dst_type_integer; return TOKEN_TYPE; }
[a-zA-Z_][a-zA-Z_0-9]*	{ yylval->string = _strdup(yytext); return TOKEN_IDENTIFIER; }

-?[0-9]+				{ sscanf(yytext, "%d", &yylval->integer); return TOKEN_INTEGER; }
-?[0-9]+"."[0-9]*		{ sscanf(yytext, "%f", &yylval->real); return TOKEN_REAL; }

[ \r\n\t]*				{  }

.						{ return yytext[0]; }

%%