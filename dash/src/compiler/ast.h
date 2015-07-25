#ifndef dash_ast_h
#define dash_ast_h

/* Dash Type */

enum dsh_type
{
	dsh_type_real = 0,
	dsh_type_integer = 1,
};
typedef enum dsh_type dsh_type;

struct dsh_type_list
{
	dsh_type		 value;

	struct dsh_type_list	*prev;
	struct dsh_type_list	*next;
};
typedef struct dsh_type_list dsh_type_list;

dsh_type_list	*dsh_alloc_type_list(dsh_type_list *list, dsh_type value);
void			 dsh_dealloc_type_list(dsh_type_list *list);

void dsh_print_type(dsh_type type, int tab_level);
void dsh_print_type_list(dsh_type_list *list, int tab_level);

/* Dash Identifier */

typedef char dsh_id;

struct dsh_id_list
{
	dsh_id		*value;

	struct dsh_id_list *prev;
	struct dsh_id_list *next;
};
typedef struct dsh_id_list dsh_id_list;

dsh_id	*dsh_grab_id(char *name);
void	 dsh_dealloc_id(dsh_id *id);

dsh_id_list *dsh_alloc_id_list(dsh_id_list *list, dsh_id *value);
void		 dsh_dealloc_id_list(dsh_id_list *list);

void dsh_print_id(dsh_id *id, int tab_level);
void dsh_print_id_list(dsh_id_list *list, int tab_level);

/* Dash Expression */

enum dsh_exp_type
{
	dsh_exp_type_variable,
	dsh_exp_type_integer,
	dsh_exp_type_real,
	dsh_exp_type_cast,
	
	dsh_exp_type_addition,
	dsh_exp_type_subtraction,
	dsh_exp_type_multiplication,
	dsh_exp_type_division,
	
	dsh_exp_type_definition,
	dsh_exp_type_assignment,

	dsh_exp_type_call,

	dsh_exp_type_block,
	dsh_exp_type_if,
	dsh_exp_type_while,
};
typedef enum dsh_exp_type dsh_exp_type;

struct dsh_exp_list;

struct dsh_exp
{
	dsh_exp_type type;

	union
	{
		struct
		{
			dsh_id *identifier;
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
			dsh_type		dest_type;
			struct dsh_exp *value;
		} cast;

		struct
		{
			struct dsh_exp *left;
			struct dsh_exp *right;
		} addition;

		struct
		{
			struct dsh_exp *left;
			struct dsh_exp *right;
		} subtraction;

		struct
		{
			struct dsh_exp *left;
			struct dsh_exp *right;
		} multiplication;

		struct
		{
			struct dsh_exp *left;
			struct dsh_exp *right;
		} division;

		struct
		{
			dsh_id_list *variables;
			struct dsh_exp_list *values;
		} definition;

		struct
		{
			dsh_id_list *variables;
			struct dsh_exp_list *values;
		} assignment;

		struct
		{
			dsh_id *function;
			struct dsh_exp_list *parameters;
		} call;

		struct
		{
			struct dsh_exp_list *statements;
		} block;

		struct
		{
			struct dsh_exp *condition;
			struct dsh_exp_list *true_exp;
			struct dsh_exp_list *false_exp;
		} if_else;

		struct
		{
			struct dsh_exp *condition;
			struct dsh_exp_list *exp;
		} while_loop;
	};
};
typedef struct dsh_exp dsh_exp;

struct dsh_exp_list
{
	dsh_exp		*value;

	struct dsh_exp_list *prev;
	struct dsh_exp_list *next;
};
typedef struct dsh_exp_list dsh_exp_list;

dsh_exp *dsh_alloc_exp_var(dsh_id *value);
dsh_exp *dsh_alloc_exp_int(int value);
dsh_exp *dsh_alloc_exp_real(float value);
dsh_exp *dsh_alloc_exp_cast(dsh_type dest_type, dsh_exp *value);

dsh_exp *dsh_alloc_exp_add(dsh_exp *left, dsh_exp *right);
dsh_exp *dsh_alloc_exp_sub(dsh_exp *left, dsh_exp *right);
dsh_exp *dsh_alloc_exp_mul(dsh_exp *left, dsh_exp *right);
dsh_exp *dsh_alloc_exp_div(dsh_exp *left, dsh_exp *right);

dsh_exp *dsh_alloc_exp_definition(dsh_id_list *variables, dsh_exp_list *assignments);
dsh_exp *dsh_alloc_exp_assignment(dsh_id_list *variables, dsh_exp_list *assignments);

dsh_exp *dsh_alloc_exp_call(dsh_id *function, dsh_exp_list *parameters);

dsh_exp *dsh_alloc_exp_block(dsh_exp_list *statements);
dsh_exp *dsh_alloc_exp_if(dsh_exp *condition, dsh_exp_list *true_exp, dsh_exp_list *false_exp);
dsh_exp *dsh_alloc_exp_while(dsh_exp *condition, dsh_exp_list *statement);

void dsh_dealloc_exp(dsh_exp *exp);

dsh_exp_list	*dsh_alloc_exp_list(dsh_exp_list *list, dsh_exp *value);
void			 dsh_dealloc_exp_list(dsh_exp_list *list);

void dsh_print_exp(dsh_exp *exp, int tab_level);
void dsh_print_exp_list(dsh_exp_list *list, int tab_level);

/* Dash Function */

struct dsh_func
{
	dsh_id		*name;
	dsh_exp_list *parameters;
	dsh_type_list		*out_types;
	dsh_exp_list	*code;
};
typedef struct dsh_func dsh_func;

struct dsh_func_list
{
	dsh_func *value;

	struct dsh_func_list *prev;
	struct dsh_func_list *next;
};
typedef struct dsh_func_list dsh_func_list;

dsh_func	*dsh_alloc_func(dsh_id *name, dsh_exp_list *parameters, dsh_type_list *out_types, dsh_exp_list *code);
void		 dsh_dealloc_func(dsh_func *func);

dsh_func_list	*dsh_alloc_func_list(dsh_func_list *list, dsh_func *value);
void			 dsh_dealloc_func_list(dsh_func_list *list);

void dsh_print_func(dsh_func *function);
void dsh_print_func_list(dsh_func_list *list);

#endif