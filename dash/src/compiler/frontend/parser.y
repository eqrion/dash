%{
	#include "../ast.h"

	int yylex (union YYSTYPE *yyval_param, struct YYLTYPE *yylloc_param, void *yyscanner);
	int yyerror(struct YYLTYPE *yylloc_param, dst_proc_list **parsed_module, void *scanner, const char *msg);
%}

%output  "parser.c"
%defines "parser.h"

%parse-param { dst_proc_list **parsed_module }
%param { void *scanner }

%define api.pure full
%locations

%union {
	dst_type			 type;
	dst_type_list		*type_list;
	char				*identifier;
	dst_id_list			*identifier_list;
	dst_statement		*statement;
	dst_statement_list	*statement_list;
	dst_exp				*expression;
	dst_exp_list		*expression_list;
	dst_proc_param		*proc_param;
	dst_proc_param_list	*proc_param_list;
	dst_proc			*proc; 
	dst_proc_list		*proc_list;

	int			 integer;
	float		 real;
	char		*string;
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
%token TOKEN_RETURN

%token TOKEN_OP_ADD
%token TOKEN_OP_SUB
%token TOKEN_OP_MUL
%token TOKEN_OP_DIV
%token TOKEN_OP_LESS
%token TOKEN_OP_LESS_EQ
%token TOKEN_OP_GREATER
%token TOKEN_OP_GREATER_EQ

%left '='
%left TOKEN_OP_LESS TOKEN_OP_LESS_EQ TOKEN_OP_GREATER TOKEN_OP_GREATER_EQ
%left ','
%left TOKEN_OP_ADD TOKEN_OP_SUB
%left TOKEN_OP_MUL TOKEN_OP_DIV
%precedence '(' ')'

%type <type>				type
%type <type_list>			type_list nonempty_type_list
%type <identifier>			identifier
%type <identifier_list>		nonempty_identifier_list
%type <statement>			statement
%type <statement_list>		statement_block nonempty_statement_block
%type <expression>			expression
%type <expression_list>		expression_list nonempty_expression_list
%type <proc_param>			proc_param
%type <proc_param_list>		nonempty_proc_param_list proc_param_list
%type <proc>				proc
%type <proc_list>			dash_module nonempty_proc_list

%start dash_module

%%

dash_module:
	nonempty_proc_list { *parsed_module = $1; }

type:
	TOKEN_TYPE { $$ = $1; }
	
identifier:
	TOKEN_IDENTIFIER { $$ = $1; }
	
statement:
	TOKEN_LET nonempty_identifier_list '=' nonempty_expression_list
	{
		$$ = dst_create_statement_definition($2, $4);
	} |
	nonempty_identifier_list '=' nonempty_expression_list
	{
		$$ = dst_create_statement_assignment($1, $3);
	} |
	identifier '(' expression_list ')'
	{
		$$ = dst_create_statement_call($1, $3);
	} |
	statement_block
	{
		$$ = dst_create_statement_block($1);
	} |
	TOKEN_IF '(' expression ')' statement TOKEN_ELSE statement
	{
		$$ = dst_create_statement_if($3, $5, $7);
	} |
	TOKEN_WHILE '(' expression ')' statement
	{
		$$ = dst_create_statement_while($3, $5);
	} |
	TOKEN_RETURN expression_list
	{
		$$ = dst_create_statement_return($2);
	}

expression:
	TOKEN_INTEGER									{ $$ = dst_create_exp_int($1); } |
	TOKEN_REAL										{ $$ = dst_create_exp_real($1); } |
	identifier										{ $$ = dst_create_exp_var($1); } |

	expression TOKEN_OP_ADD expression				{ $$ = dst_create_exp_add($1, $3); } |
	expression TOKEN_OP_SUB expression				{ $$ = dst_create_exp_sub($1, $3); } |
	expression TOKEN_OP_MUL expression				{ $$ = dst_create_exp_mul($1, $3); } |
	expression TOKEN_OP_DIV expression				{ $$ = dst_create_exp_div($1, $3); } |
	
	expression TOKEN_OP_LESS expression				{ $$ = dst_create_exp_cmp_l($1, $3); } |
	expression TOKEN_OP_LESS_EQ expression			{ $$ = dst_create_exp_cmp_le($1, $3); } |
	expression TOKEN_OP_GREATER expression			{ $$ = dst_create_exp_cmp_g($1, $3); } |
	expression TOKEN_OP_GREATER_EQ expression		{ $$ = dst_create_exp_cmp_le($1, $3); } |
	
	identifier '(' expression_list ')'	{ $$ = dst_create_exp_call($1, $3); } |
	'(' type ')' expression				{ $$ = dst_create_exp_cast($2, $4); } |
	'(' expression ')'					{ $$ = $2; }
		
proc_param:
	identifier ':' type { $$ = dst_create_func_param($1, $3); }

proc:
	TOKEN_DEF identifier ':' '(' proc_param_list ')' TOKEN_ARROW '(' type_list ')' statement
	{
		$$ = dst_create_func($2, $5, $9, $11);
	}
		
statement_block:
	'{' '}'								{ $$ = NULL; } |
	'{' nonempty_statement_block '}'	{ $$ = $2; }

expression_list:
	%empty						{ $$ = NULL; } |
	nonempty_expression_list	{ $$ = $1; }
	
type_list:
	%empty				{ $$ = NULL; } |
	nonempty_type_list	{ $$ = $1; }
	
proc_param_list:
	%empty						{ $$ = NULL; } |
	nonempty_proc_param_list	{ $$ = $1; }

nonempty_statement_block:
	statement ';'								{ $$ = dst_append_statement_list(NULL, $1); } |
	nonempty_statement_block statement ';'		{ $$ = dst_append_statement_list($1, $2); }

nonempty_expression_list:
	expression								{ $$ = dst_append_exp_list(NULL, $1); } |
	nonempty_expression_list ',' expression	{ $$ = dst_append_exp_list($1, $3); }
	
nonempty_type_list:
	type						{ $$ = dst_append_type_list(NULL, $1); } |
	nonempty_type_list ',' type { $$ = dst_append_type_list($1, $3); }
	
nonempty_proc_param_list:
	proc_param									{ $$ = dst_append_func_param_list(NULL, $1); } |
	nonempty_proc_param_list ',' proc_param		{ $$ = dst_append_func_param_list($1, $3); }

nonempty_identifier_list:
	identifier								{ $$ = dst_append_id_list(NULL, $1); } |
	nonempty_identifier_list ',' identifier { $$ = dst_append_id_list($1, $3); }
	
nonempty_proc_list:
	proc						{ $$ = dst_append_func_list(NULL, $1); } |
	nonempty_proc_list proc		{ $$ = dst_append_func_list($1, $2); }

%%

#include <stdio.h>

int yyerror(struct YYLTYPE *yylloc_param, dst_proc_list **parsed_module, void *scanner, const char *msg)
{
    fprintf(stderr, "error (%d:%d) - (%d:%d): %s\n",
		yylloc_param->first_line,
		yylloc_param->first_column, 
		yylloc_param->last_line,
		yylloc_param->last_column,
		msg);

	return 0;
}