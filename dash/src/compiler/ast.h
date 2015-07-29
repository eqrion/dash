#ifndef dash_ast_h
#define dash_ast_h

#include <stdint.h>

enum ast_type
{
	ast_type_real = 0,
	ast_type_integer = 1,
};

enum ast_statement_type
{
	ast_statement_type_definition,
	ast_statement_type_assignment,

	ast_statement_type_call,

	ast_statement_type_block,
	ast_statement_type_if,
	ast_statement_type_while,

	ast_statement_type_return,
};
struct ast_statement
{
	enum ast_statement_type type;

	struct
	{
		struct ast_id_list	*variables;
		struct ast_exp_list *values;
	} definition;

	struct
	{
		struct ast_id_list	*variables;
		struct ast_exp_list *values;
	} assignment;

	struct
	{
		char				*function;
		struct ast_exp_list *parameters;
	} call;

	struct
	{
		struct ast_statement_list *statements;
	} block;

	struct
	{
		struct ast_exp		*condition;
		struct ast_statement *true_statement;
		struct ast_statement *false_statement;
	} if_else;

	struct
	{
		struct ast_exp		*condition;
		struct ast_statement *loop_statement;
	} while_loop;

	struct
	{
		struct ast_exp_list		*values;
	} ret;
};
enum ast_exp_type
{
	ast_exp_type_variable,
	ast_exp_type_integer,
	ast_exp_type_real,

	ast_exp_type_cast,

	ast_exp_type_addition,
	ast_exp_type_subtraction,
	ast_exp_type_multiplication,
	ast_exp_type_division,

	ast_exp_type_less,
	ast_exp_type_less_eq,
	ast_exp_type_greater,
	ast_exp_type_greater_eq,

	ast_exp_type_call,
};
struct ast_exp
{
	enum ast_exp_type	type;

	union
	{
		struct
		{
			char *id;
		} variable;

		struct
		{
			int value;
		} integer;

		struct
		{
			float value;
		} real;

		struct
		{
			enum ast_type	dest_type;
			struct ast_exp *value;
		} cast;

		struct
		{
			struct ast_exp *left;
			struct ast_exp *right;
		} operator;

		struct
		{
			char				*function;
			struct ast_exp_list *parameters;
		} call;
	};

	size_t temp_count_est;
};
struct ast_func_param
{
	char			*id;
	enum ast_type	 type;
};
struct ast_func
{
	char							*id;
	struct ast_func_param_list		*in_params;
	struct ast_type_list			*out_types;
	struct ast_statement			*statement;
};
typedef enum ast_type				ast_type;
typedef enum ast_statement_type		ast_statement_type;
typedef enum ast_exp_type			ast_exp_type;
typedef struct ast_statement		ast_statement;
typedef struct ast_exp				ast_exp;
typedef struct ast_func_param		ast_func_param;
typedef struct ast_func				ast_func;

/* Lists */

struct ast_type_list
{
	ast_type		 value;

	struct ast_type_list	*prev;
	struct ast_type_list	*next;
};
struct ast_id_list
{
	char		*value;

	struct ast_id_list *prev;
	struct ast_id_list *next;
};
struct ast_statement_list
{
	ast_statement		*value;

	struct ast_statement_list *prev;
	struct ast_statement_list *next;
};
struct ast_exp_list
{
	ast_exp		*value;

	struct ast_exp_list *prev;
	struct ast_exp_list *next;
};
struct ast_func_param_list
{
	ast_func_param *value;

	struct ast_func_param_list *prev;
	struct ast_func_param_list *next;
};
struct ast_func_list
{
	ast_func *value;

	struct ast_func_list *prev;
	struct ast_func_list *next;
};

typedef struct ast_type_list		ast_type_list;
typedef struct ast_id_list			ast_id_list;
typedef struct ast_statement_list	ast_statement_list;
typedef struct ast_exp_list			ast_exp_list;
typedef struct ast_func_list		ast_func_list;
typedef struct ast_func_param_list	ast_func_param_list;

static ast_type_list ast_sentinel_type_real = { ast_type_real, &ast_sentinel_type_real, &ast_sentinel_type_real };
static ast_type_list ast_sentinel_type_integer = { ast_type_integer, &ast_sentinel_type_integer, &ast_sentinel_type_integer };

/* Constructors, Destructors, Accessors */

ast_statement *ast_create_statement_definition(ast_id_list *variables, ast_exp_list *assignments);
ast_statement *ast_create_statement_assignment(ast_id_list *variables, ast_exp_list *assignments);
ast_statement *ast_create_statement_call(char *function, ast_exp_list *parameters);
ast_statement *ast_create_statement_block(ast_statement_list *statements);
ast_statement *ast_create_statement_if(ast_exp *condition, ast_statement *true_statement, ast_statement *false_statement);
ast_statement *ast_create_statement_while(ast_exp *condition, ast_statement *loop_statement);
ast_statement *ast_create_statement_return(ast_exp_list *value);

ast_exp *ast_create_exp_var(char *value);
ast_exp *ast_create_exp_int(int value);
ast_exp *ast_create_exp_real(float value);
ast_exp *ast_create_exp_cast(ast_type dest_type, ast_exp *value);
ast_exp *ast_create_exp_add(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_sub(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_mul(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_div(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_cmp_l(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_cmp_le(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_cmp_g(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_cmp_ge(ast_exp *left, ast_exp *right);
ast_exp *ast_create_exp_call(char *function, ast_exp_list *parameters);

ast_func_param	*ast_create_func_param(char *id, ast_type type);
ast_func		*ast_create_func(char *id, ast_func_param_list *in_params, ast_type_list *out_types, ast_statement *statement);

void	ast_destroy_statement(ast_statement *statement);
void	ast_destroy_exp(ast_exp *exp);
void	ast_destroy_func_param(ast_func_param *func_param);
void	ast_destroy_func(ast_func *func);

ast_type_list		*ast_append_type_list(ast_type_list *list, ast_type value);
ast_id_list			*ast_append_id_list(ast_id_list *list, char *value);
ast_statement_list	*ast_append_statement_list(ast_statement_list *list, ast_statement *value);
ast_exp_list		*ast_append_exp_list(ast_exp_list *list, ast_exp *value);
ast_func_param_list	*ast_append_func_param_list(ast_func_param_list *list, ast_func_param *value);
ast_func_list		*ast_append_func_list(ast_func_list *list, ast_func *value);

void			 ast_destroy_type_list(ast_type_list *list);
void			 ast_destroy_id_list(ast_id_list *list);
void			 ast_destroy_statement_list(ast_statement_list *list);
void			 ast_destroy_exp_list(ast_exp_list *list);
void			 ast_destroy_func_param_list(ast_func_param_list *list);
void			 ast_destroy_func_list(ast_func_list *list);

size_t ast_exp_list_count(ast_exp_list *list);
size_t ast_type_list_count(ast_type_list *list);
size_t ast_func_param_list_count(ast_func_param_list *list);

/* Printing */

void ast_print_type(ast_type type, int tab_level);
void ast_print_id(char *id, int tab_level);
void ast_print_statement(ast_statement *statement, int tab_level);
void ast_print_exp(ast_exp *exp, int tab_level);
void ast_print_func_param(ast_func_param *function_param, int tab_level);
void ast_print_func(ast_func *function);

void ast_print_type_list(ast_type_list *list, int tab_level);
void ast_print_id_list(ast_id_list *list, int tab_level);
void ast_print_statement_list(ast_statement_list *list, int tab_level);
void ast_print_exp_list(ast_exp_list *list, int tab_level);
void ast_print_func_param_list(ast_func_param_list *list, int tab_level);
void ast_print_func_list(ast_func_list *list);

#endif