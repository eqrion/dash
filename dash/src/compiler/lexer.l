%{
	#include "ast.h"
	#include "parser.h"
%}

%option outfile="lexer.c"
%option warn nodefault nounistd never-interactive
 
%option reentrant noyywrap yylineno
%option bison-bridge bison-locations

%%

[+\-\*/{}:;(),] { return yytext[0]; }

"def"		{ return TOKEN_DEF; }
"->"		{ return TOKEN_ARROW; }
"let"		{ return TOKEN_LET; }
"while"		{ return TOKEN_WHILE; }
"if"		{ return TOKEN_IF; }
"else"		{ return TOKEN_ELSE; }

"real"		{ yylval->type = dsh_type_real; return TOKEN_TYPE; }
"integer"	{ yylval->type = dsh_type_integer; return TOKEN_TYPE; }
[a-zA-Z_][a-zA-Z_0-9]*	{ yylval->string = _strdup(yytext); return TOKEN_IDENTIFIER; }

-?[0-9]+				{ sscanf(yytext, "%d", &yylval->integer); return TOKEN_INTEGER; }
-?[0-9]+"."[0-9]*		{ sscanf(yytext, "%f", &yylval->real); return TOKEN_REAL; }

[ \r\n\t]*				{  }
.						{  }
 
%%
	