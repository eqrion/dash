%{
	#include "ast.h"

	int yylex (union YYSTYPE *yyval_param, struct YYLTYPE *yylloc_param, void *yyscanner);
	int yyerror(struct YYLTYPE *yylloc_param, dsh_func_list **parsed_module, void *scanner, const char *msg);
%}

%output  "parser.c"
%defines "parser.h"

%parse-param { dsh_func_list **parsed_module }
%param {void *scanner}
%define parse.trace
%define api.pure full
%locations

%union {
	dsh_type		 type;
	dsh_type_list	*type_list;
	dsh_id			*identifier;
	dsh_id_list		*identifier_list;
	dsh_exp			*expression;
	dsh_exp_list	*expression_list;
	dsh_func		*function;
	dsh_func_list	*function_list;

	int			integer;
	float		real;
	char*		string;
}
%token <integer> TOKEN_INTEGER		
%token <real> TOKEN_REAL			
%token <string> TOKEN_IDENTIFIER	
%token <type> TOKEN_TYPE			
%token TOKEN_DEF					
%token TOKEN_ARROW					
%token TOKEN_LET					
%token TOKEN_WHILE					
%token TOKEN_IF						
%token TOKEN_ELSE					

%precedence ','
%left '+' '-'
%left '*' '/'

%type <type> type
%type <type_list> type_nonempty_list
%type <identifier> identifier
%type <identifier_list> identifier_nonempty_list
%type <expression> expression
%type <expression_list> expression_block expression_nonempty_block expression_list expression_nonempty_list
%type <function> function
%type <function_list> function_nonempty_list
%type <function_list> dash_module

%start dash_module

%%

dash_module:
	function_nonempty_list { *parsed_module = $1; }

type:
	TOKEN_TYPE { $$ = $1; }

type_nonempty_list:
	type { $$ = dsh_alloc_type_list(NULL, $1); } |
	type_nonempty_list ',' type { $$ = dsh_alloc_type_list($1, $3); }

identifier:
	TOKEN_IDENTIFIER { $$ = dsh_grab_id($1); }

identifier_nonempty_list:
	identifier { $$ = dsh_alloc_id_list(NULL, $1); } |
	identifier_nonempty_list ',' identifier { $$ = dsh_alloc_id_list($1, $3); }

expression:
	identifier										{ $$ = dsh_alloc_exp_var($1); } |
	TOKEN_INTEGER									{ $$ = dsh_alloc_exp_int($1); } |
	TOKEN_REAL										{ $$ = dsh_alloc_exp_real($1); } |
	expression '+' expression						{ $$ = dsh_alloc_exp_add($1, $3); } |
	expression '-' expression						{ $$ = dsh_alloc_exp_sub($1, $3); } |
	expression '*' expression						{ $$ = dsh_alloc_exp_mul($1, $3); } |
	expression '/' expression						{ $$ = dsh_alloc_exp_div($1, $3); } |	
	TOKEN_LET '(' identifier_nonempty_list ')' '=' '(' expression_nonempty_list ')'
	{
		$$ = dsh_alloc_exp_definition($3, $7);
	} |
	'(' identifier_nonempty_list ')' '=' '(' expression_nonempty_list ')'
	{
		$$ = dsh_alloc_exp_assignment($2, $6);
	} |
	identifier '(' expression_list ')'
	{
		$$ = dsh_alloc_exp_call($1, $3);
	} |
	'{' expression_block '}'
	{
		$$ = dsh_alloc_exp_block($2);
	} |
	TOKEN_IF '(' expression ')' '{' expression_block '}' TOKEN_ELSE '{' expression_block '}'
	{
		$$ = dsh_alloc_exp_if($3, $6, $10);
	} |
	TOKEN_WHILE '(' expression ')' '{' expression_block '}'
	{
		$$ = dsh_alloc_exp_while($3, $6);
	}
	
expression_block:
	%empty { $$ = NULL; } |
	expression_nonempty_block { $$ = $1; }
	
expression_nonempty_block:
	expression ';' { $$ = dsh_alloc_exp_list(NULL, $1); } |
	expression_nonempty_block expression ';' { $$ = dsh_alloc_exp_list($1, $2); }

expression_list:
	%empty { $$ = NULL; } |
	expression_nonempty_list { $$ = $1; }

expression_nonempty_list:
	expression { $$ = dsh_alloc_exp_list(NULL, $1); } |
	expression_nonempty_list ',' expression { $$ = dsh_alloc_exp_list($1, $3); }

function:
	TOKEN_DEF identifier ':' '(' expression_list ')' TOKEN_ARROW '(' type_nonempty_list ')' '{' expression_block '}'
	{
		$$ = dsh_alloc_func($2, $5, $9, $12);
	}

function_nonempty_list:
	function { $$ = dsh_alloc_func_list(NULL, $1); } |
	function_nonempty_list function { $$ = dsh_alloc_func_list($1, $2); }

%%

#include <stdio.h>

int yyerror(struct YYLTYPE *yylloc_param, dsh_func_list **parsed_module, void *scanner, const char *msg)
{
    fprintf(stderr, "error (%d:%d) - (%d:%d): %s\n",
		yylloc_param->first_line,
		yylloc_param->first_column, 
		yylloc_param->last_line,
		yylloc_param->last_column,
		msg);

	return 0;
}