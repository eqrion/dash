%{
	#include "ast.h"

	int yylex (union YYSTYPE *yyval_param, struct YYLTYPE *yylloc_param, void *yyscanner);
	int yyerror(struct YYLTYPE *yylloc_param, ast_func_list **parsed_module, void *scanner, const char *msg);
%}

%output  "parser.c"
%defines "parser.h"

%parse-param { ast_func_list **parsed_module }
%param { void *scanner }

%define api.pure full
%locations

%union {
	ast_type			 type;
	ast_type_list		*type_list;
	char				*identifier;
	ast_id_list			*identifier_list;
	ast_statement		*statement;
	ast_statement_list	*statement_list;
	ast_exp				*expression;
	ast_exp_list		*expression_list;
	ast_func_param		*function_param;
	ast_func_param_list	*function_param_list;
	ast_func			*function; 
	ast_func_list		*function_list;

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
%type <function_param>		function_param
%type <function_param_list>	nonempty_function_param_list function_param_list
%type <function>			function
%type <function_list>		dash_module nonempty_function_list

%start dash_module

%%

dash_module:
	nonempty_function_list { *parsed_module = $1; }

type:
	TOKEN_TYPE { $$ = $1; }
	
identifier:
	TOKEN_IDENTIFIER { $$ = $1; }
	
statement:
	TOKEN_LET nonempty_identifier_list '=' nonempty_expression_list
	{
		$$ = ast_create_statement_definition($2, $4);
	} |
	nonempty_identifier_list '=' nonempty_expression_list
	{
		$$ = ast_create_statement_assignment($1, $3);
	} |
	identifier '(' expression_list ')'
	{
		$$ = ast_create_statement_call($1, $3);
	} |
	statement_block
	{
		$$ = ast_create_statement_block($1);
	} |
	TOKEN_IF '(' expression ')' statement TOKEN_ELSE statement
	{
		$$ = ast_create_statement_if($3, $5, $7);
	} |
	TOKEN_WHILE '(' expression ')' statement
	{
		$$ = ast_create_statement_while($3, $5);
	} |
	TOKEN_RETURN expression_list
	{
		$$ = ast_create_statement_return($2);
	}

expression:
	TOKEN_INTEGER									{ $$ = ast_create_exp_int($1); } |
	TOKEN_REAL										{ $$ = ast_create_exp_real($1); } |
	identifier										{ $$ = ast_create_exp_var($1); } |

	expression TOKEN_OP_ADD expression				{ $$ = ast_create_exp_add($1, $3); } |
	expression TOKEN_OP_SUB expression				{ $$ = ast_create_exp_sub($1, $3); } |
	expression TOKEN_OP_MUL expression				{ $$ = ast_create_exp_mul($1, $3); } |
	expression TOKEN_OP_DIV expression				{ $$ = ast_create_exp_div($1, $3); } |
	
	expression TOKEN_OP_LESS expression				{ $$ = ast_create_exp_cmp_l($1, $3); } |
	expression TOKEN_OP_LESS_EQ expression			{ $$ = ast_create_exp_cmp_le($1, $3); } |
	expression TOKEN_OP_GREATER expression			{ $$ = ast_create_exp_cmp_g($1, $3); } |
	expression TOKEN_OP_GREATER_EQ expression		{ $$ = ast_create_exp_cmp_le($1, $3); } |
	
	identifier '(' expression_list ')' { $$ = ast_create_exp_call($1, $3); } |
	'(' type ')' expression { $$ = ast_create_exp_cast($2, $4); } |
	'(' expression ')' { $$ = $2; }
		
function_param:
	identifier ':' type { $$ = ast_create_func_param($1, $3); }

function:
	TOKEN_DEF identifier ':' '(' function_param_list ')' TOKEN_ARROW '(' type_list ')' statement
	{
		$$ = ast_create_func($2, $5, $9, $11);
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
	
function_param_list:
	%empty							{ $$ = NULL; } |
	nonempty_function_param_list	{ $$ = $1; }

nonempty_statement_block:
	statement ';'								{ $$ = ast_append_statement_list(NULL, $1); } |
	nonempty_statement_block statement ';'		{ $$ = ast_append_statement_list($1, $2); }

nonempty_expression_list:
	expression								{ $$ = ast_append_exp_list(NULL, $1); } |
	nonempty_expression_list ',' expression	{ $$ = ast_append_exp_list($1, $3); }
	
nonempty_type_list:
	type						{ $$ = ast_append_type_list(NULL, $1); } |
	nonempty_type_list ',' type { $$ = ast_append_type_list($1, $3); }
	
nonempty_function_param_list:
	function_param									{ $$ = ast_append_func_param_list(NULL, $1); } |
	nonempty_function_param_list ',' function_param { $$ = ast_append_func_param_list($1, $3); }

nonempty_identifier_list:
	identifier								{ $$ = ast_append_id_list(NULL, $1); } |
	nonempty_identifier_list ',' identifier { $$ = ast_append_id_list($1, $3); }
	
nonempty_function_list:
	function						{ $$ = ast_append_func_list(NULL, $1); } |
	nonempty_function_list function { $$ = ast_append_func_list($1, $2); }

%%

#include <stdio.h>

int yyerror(struct YYLTYPE *yylloc_param, ast_func_list **parsed_module, void *scanner, const char *msg)
{
    fprintf(stderr, "error (%d:%d) - (%d:%d): %s\n",
		yylloc_param->first_line,
		yylloc_param->first_column, 
		yylloc_param->last_line,
		yylloc_param->last_column,
		msg);

	return 0;
}