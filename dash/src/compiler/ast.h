#ifndef dash_dst_h
#define dash_dst_h

#include <stdint.h>
#include "memory.h"

enum dst_type
{
	dst_type_real = 0,
	dst_type_integer = 1,
};

enum dst_statement_type
{
	dst_statement_type_definition,
	dst_statement_type_assignment,
	
	dst_statement_type_call,

	dst_statement_type_block,
	dst_statement_type_if,
	dst_statement_type_while,

	dst_statement_type_return,
};
struct dst_statement
{
	enum dst_statement_type type;

	struct
	{
		struct dst_id_list	*variables;
		struct dst_exp_list *values;
	} definition;

	struct
	{
		struct dst_id_list	*variables;
		struct dst_exp_list *values;
	} assignment;

	struct
	{
		char				*function;
		struct dst_exp_list *parameters;
	} call;

	struct
	{
		struct dst_statement_list *statements;
	} block;

	struct
	{
		struct dst_exp		*condition;
		struct dst_statement *true_statement;
		struct dst_statement *false_statement;
	} if_else;

	struct
	{
		struct dst_exp		*condition;
		struct dst_statement *loop_statement;
	} while_loop;

	struct
	{
		struct dst_exp_list		*values;
	} ret;
};
enum dst_exp_type
{
	dst_exp_type_variable,
	dst_exp_type_integer,
	dst_exp_type_real,

	dst_exp_type_cast,

	dst_exp_type_addition,
	dst_exp_type_subtraction,
	dst_exp_type_multiplication,
	dst_exp_type_division,

	dst_exp_type_and,
	dst_exp_type_or,
	dst_exp_type_not,

	dst_exp_type_eq,
	dst_exp_type_less,
	dst_exp_type_less_eq,
	dst_exp_type_greater,
	dst_exp_type_greater_eq,

	dst_exp_type_call,
};
struct dst_exp
{
	enum dst_exp_type	type;

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
			enum dst_type	dest_type;
			struct dst_exp *value;
		} cast;

		struct
		{
			struct dst_exp *value;
		} unary;

		struct
		{
			struct dst_exp *left;
			struct dst_exp *right;
		} binary;

		struct
		{
			char				*function;
			struct dst_exp_list *parameters;
		} call;
	};

	size_t temp_count_est;
};
struct dst_proc_param
{
	char			*id;
	enum dst_type	 type;
};
struct dst_proc
{
	char							*id;
	struct dst_proc_param_list		*in_params;
	struct dst_type_list			*out_types;
	struct dst_statement			*statement;
};
typedef enum dst_type				dst_type;
typedef enum dst_statement_type		dst_statement_type;
typedef enum dst_exp_type			dst_exp_type;
typedef struct dst_statement		dst_statement;
typedef struct dst_exp				dst_exp;
typedef struct dst_proc_param		dst_proc_param;
typedef struct dst_proc				dst_proc;

/* Lists */

struct dst_type_list
{
	dst_type		 value;

	struct dst_type_list	*prev;
	struct dst_type_list	*next;
};
struct dst_id_list
{
	char		*value;

	struct dst_id_list *prev;
	struct dst_id_list *next;
};
struct dst_statement_list
{
	dst_statement		*value;

	struct dst_statement_list *prev;
	struct dst_statement_list *next;
};
struct dst_exp_list
{
	dst_exp		*value;

	struct dst_exp_list *prev;
	struct dst_exp_list *next;
};
struct dst_proc_param_list
{
	dst_proc_param *value;

	struct dst_proc_param_list *prev;
	struct dst_proc_param_list *next;
};
struct dst_proc_list
{
	dst_proc *value;

	struct dst_proc_list *prev;
	struct dst_proc_list *next;
};

typedef struct dst_type_list		dst_type_list;
typedef struct dst_id_list			dst_id_list;
typedef struct dst_statement_list	dst_statement_list;
typedef struct dst_exp_list			dst_exp_list;
typedef struct dst_proc_list		dst_proc_list;
typedef struct dst_proc_param_list	dst_proc_param_list;

static dst_type_list dst_sentinel_type_real = { dst_type_real, &dst_sentinel_type_real, &dst_sentinel_type_real };
static dst_type_list dst_sentinel_type_integer = { dst_type_integer, &dst_sentinel_type_integer, &dst_sentinel_type_integer };

/* Constructors, Destructors, Accessors */

dst_statement *dst_create_statement_definition(dst_id_list *variables, dst_exp_list *assignments, dsc_memory *mem);
dst_statement *dst_create_statement_assignment(dst_id_list *variables, dst_exp_list *assignments, dsc_memory *mem);
dst_statement *dst_create_statement_call(char *function, dst_exp_list *parameters, dsc_memory *mem);
dst_statement *dst_create_statement_block(dst_statement_list *statements, dsc_memory *mem);
dst_statement *dst_create_statement_if(dst_exp *condition, dst_statement *true_statement, dst_statement *false_statement, dsc_memory *mem);
dst_statement *dst_create_statement_while(dst_exp *condition, dst_statement *loop_statement, dsc_memory *mem);
dst_statement *dst_create_statement_return(dst_exp_list *value, dsc_memory *mem);

dst_exp *dst_create_exp_var(char *value, dsc_memory *mem);
dst_exp *dst_create_exp_int(int value, dsc_memory *mem);
dst_exp *dst_create_exp_real(float value, dsc_memory *mem);
dst_exp *dst_create_exp_cast(dst_type dest_type, dst_exp *value, dsc_memory *mem);
dst_exp *dst_create_exp_binary(dst_exp_type type, dst_exp *left, dst_exp *right, dsc_memory *mem);
dst_exp *dst_create_exp_unary(dst_exp_type type, dst_exp *value, dsc_memory *mem);
dst_exp *dst_create_exp_call(char *function, dst_exp_list *parameters, dsc_memory *mem);

dst_proc_param	*dst_create_proc_param(char *id, dst_type type, dsc_memory *mem);
dst_proc		*dst_create_proc(char *id, dst_proc_param_list *in_params, dst_type_list *out_types, dst_statement *statement, dsc_memory *mem);

dst_type_list		*dst_append_type_list(dst_type_list *list, dst_type value, dsc_memory *mem);
dst_id_list			*dst_append_id_list(dst_id_list *list, char *value, dsc_memory *mem);
dst_statement_list	*dst_append_statement_list(dst_statement_list *list, dst_statement *value, dsc_memory *mem);
dst_exp_list		*dst_append_exp_list(dst_exp_list *list, dst_exp *value, dsc_memory *mem);
dst_proc_param_list	*dst_append_func_param_list(dst_proc_param_list *list, dst_proc_param *value, dsc_memory *mem);
dst_proc_list		*dst_append_func_list(dst_proc_list *list, dst_proc *value, dsc_memory *mem);

size_t dst_exp_list_count(dst_exp_list *list);
size_t dst_type_list_count(dst_type_list *list);
size_t dst_proc_param_list_count(dst_proc_param_list *list);

int dst_type_list_is_integer(dst_type_list *list);
int dst_type_list_is_real(dst_type_list *list);
int dst_type_list_is_composite(dst_type_list *list);

#endif