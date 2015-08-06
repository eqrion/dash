#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Constructors, Destructors, Accessors */

dst_statement *dst_create_statement_definition(dst_id_list *variables, dst_exp_list *assignments, dsc_memory *mem)
{
	dst_statement *statement = (dst_statement *)dsc_alloc(sizeof(dst_statement), mem);

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_definition;
	statement->definition.variables = variables;
	statement->definition.values = assignments;

	return statement;
}
dst_statement *dst_create_statement_assignment(dst_id_list *variables, dst_exp_list *assignments, dsc_memory *mem)
{
	dst_statement *statement = (dst_statement *)dsc_alloc(sizeof(dst_statement), mem);

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_assignment;
	statement->assignment.variables = variables;
	statement->assignment.values = assignments;

	return statement;
}
dst_statement *dst_create_statement_call(char *function, dst_exp_list *parameters, dsc_memory *mem)
{
	if (function == NULL || parameters == NULL)
		return NULL;

	dst_statement *statement = (dst_statement *)dsc_alloc(sizeof(dst_statement), mem);

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_call;
	statement->call.function = function;
	statement->call.parameters = parameters;

	return statement;
}
dst_statement *dst_create_statement_block(dst_statement_list *statements, dsc_memory *mem)
{
	dst_statement *statement = (dst_statement *)dsc_alloc(sizeof(dst_statement), mem);

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_block;
	statement->block.statements = statements;

	return statement;
}
dst_statement *dst_create_statement_if(dst_exp *condition, dst_statement *true_statement, dst_statement *false_statement, dsc_memory *mem)
{
	if (condition == NULL || true_statement == NULL || false_statement == NULL)
		return NULL;

	dst_statement *statement = (dst_statement *)dsc_alloc(sizeof(dst_statement), mem);

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_if;
	statement->if_else.condition = condition;
	statement->if_else.true_statement = true_statement;
	statement->if_else.false_statement = false_statement;

	return statement;
}
dst_statement *dst_create_statement_while(dst_exp *condition, dst_statement *loop_statement, dsc_memory *mem)
{
	if (condition == NULL || loop_statement == NULL)
		return NULL;

	dst_statement *statement = (dst_statement *)dsc_alloc(sizeof(dst_statement), mem);

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_while;
	statement->while_loop.condition = condition;
	statement->while_loop.loop_statement = loop_statement;

	return statement;
}
dst_statement *dst_create_statement_return(dst_exp_list *values, dsc_memory *mem)
{
	if (values == NULL)
		return NULL;

	dst_statement *statement = (dst_statement *)dsc_alloc(sizeof(dst_statement), mem);

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_return;
	statement->ret.values = values;

	return statement;
}

dst_exp *dst_create_exp_var(char *value, dsc_memory *mem)
{
	dst_exp *exp = (dst_exp *)dsc_alloc(sizeof(dst_exp), mem);

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_variable;
	exp->variable.id = value;
	exp->temp_count_est = 0;

	return exp;
}
dst_exp *dst_create_exp_int(int value, dsc_memory *mem)
{
	dst_exp *exp = (dst_exp *)dsc_alloc(sizeof(dst_exp), mem);

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_integer;
	exp->integer.value = value;
	exp->temp_count_est = 1;

	return exp;
}
dst_exp *dst_create_exp_real(float value, dsc_memory *mem)
{
	dst_exp *exp = (dst_exp *)dsc_alloc(sizeof(dst_exp), mem);

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_real;
	exp->real.value = value;
	exp->temp_count_est = 1;

	return exp;
}
dst_exp *dst_create_exp_cast(dst_type dest_type, dst_exp *value, dsc_memory *mem)
{
	if (value == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)dsc_alloc(sizeof(dst_exp), mem);

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_cast;
	exp->cast.dest_type = dest_type;
	exp->cast.value = value;
	exp->temp_count_est = max(value->temp_count_est, 1);

	return exp;
}
dst_exp *dst_create_exp_binary(dst_exp_type type, dst_exp *left, dst_exp *right, dsc_memory *mem)
{
	if (left == NULL || right == NULL)
		return NULL;

	if (type != dst_exp_type_addition &&
		type != dst_exp_type_subtraction &&
		type != dst_exp_type_multiplication &&
		type != dst_exp_type_division &&
		type != dst_exp_type_and &&
		type != dst_exp_type_or &&
		type != dst_exp_type_eq &&
		type != dst_exp_type_less &&
		type != dst_exp_type_less_eq &&
		type != dst_exp_type_greater &&
		type != dst_exp_type_greater_eq)
	{
		return NULL;
	}

	dst_exp *exp = (dst_exp *)dsc_alloc(sizeof(dst_exp), mem);

	if (exp == NULL)
		return NULL;

	exp->type = type;
	exp->binary.left = left;
	exp->binary.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_unary(dst_exp_type type, dst_exp *value, dsc_memory *mem)
{
	if (value == NULL)
		return NULL;

	if (type != dst_exp_type_not)
	{
		return NULL;
	}

	dst_exp *exp = (dst_exp *)dsc_alloc(sizeof(dst_exp), mem);

	if (exp == NULL)
		return NULL;

	exp->type = type;
	exp->unary.value = value;
	exp->temp_count_est = value->temp_count_est;

	return exp;
}
dst_exp *dst_create_exp_call(char *function, dst_exp_list *parameters, dsc_memory *mem)
{
	dst_exp *exp = (dst_exp *)dsc_alloc(sizeof(dst_exp), mem);

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_call;
	exp->call.function = function;
	exp->call.parameters = parameters;
	exp->temp_count_est = max(dst_exp_list_count(parameters), 1);

	return exp;
}

dst_proc_param	*dst_create_proc_param(char *id, dst_type type, dsc_memory *mem)
{
	dst_proc_param *func_param = (dst_proc_param *)dsc_alloc(sizeof(dst_proc_param), mem);

	if (func_param == NULL)
		return NULL;
	
	func_param->id = id;
	func_param->type = type;

	return func_param;
}
dst_proc *dst_create_proc(char *id, dst_proc_param_list *in_params, dst_type_list *out_types, dst_statement *code, dsc_memory *mem)
{
	dst_proc *func = (dst_proc *)dsc_alloc(sizeof(dst_proc), mem);

	if (func == NULL)
		return NULL;

	func->id = id;
	func->in_params = in_params;
	func->out_types = out_types;
	func->statement = code;

	return func;
}

dst_type_list		*dst_append_type_list(dst_type_list *list, dst_type value, dsc_memory *mem)
{
	if (list == NULL)
	{
		list = (dst_type_list *)dsc_alloc(sizeof(dst_type_list), mem);

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_type_list *next = (dst_type_list *)dsc_alloc(sizeof(dst_type_list), mem);

		if (next == NULL)
			return NULL;

		next->value = value;
		next->next = list;
		next->prev = list->prev;

		list->prev->next = next;
		list->prev = next;
	}

	return list;
}
dst_id_list			*dst_append_id_list(dst_id_list *list, char *value, dsc_memory *mem)
{
	if (list == NULL)
	{
		list = (dst_id_list *)dsc_alloc(sizeof(dst_id_list), mem);

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_id_list *next = (dst_id_list *)dsc_alloc(sizeof(dst_id_list), mem);

		if (next == NULL)
			return NULL;

		next->value = value;
		next->next = list;
		next->prev = list->prev;

		list->prev->next = next;
		list->prev = next;
	}

	return list;
}
dst_statement_list	*dst_append_statement_list(dst_statement_list *list, dst_statement *value, dsc_memory *mem)
{
	if (list == NULL)
	{
		list = (dst_statement_list *)dsc_alloc(sizeof(dst_statement_list), mem);

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_statement_list *next = (dst_statement_list *)dsc_alloc(sizeof(dst_statement_list), mem);

		if (next == NULL)
			return NULL;

		next->value = value;
		next->next = list;
		next->prev = list->prev;

		list->prev->next = next;
		list->prev = next;
	}

	return list;
}
dst_exp_list		*dst_append_exp_list(dst_exp_list *list, dst_exp *value, dsc_memory *mem)
{
	if (list == NULL)
	{
		list = (dst_exp_list *)dsc_alloc(sizeof(dst_exp_list), mem);

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_exp_list *next = (dst_exp_list *)dsc_alloc(sizeof(dst_exp_list), mem);

		if (next == NULL)
			return NULL;

		next->value = value;
		next->next = list;
		next->prev = list->prev;

		list->prev->next = next;
		list->prev = next;
	}

	return list;
}
dst_proc_param_list	*dst_append_func_param_list(dst_proc_param_list *list, dst_proc_param *value, dsc_memory *mem)
{
	if (list == NULL)
	{
		list = (dst_proc_param_list *)dsc_alloc(sizeof(dst_proc_param_list), mem);

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_proc_param_list *next = (dst_proc_param_list *)dsc_alloc(sizeof(dst_proc_param_list), mem);

		if (next == NULL)
			return NULL;

		next->value = value;
		next->next = list;
		next->prev = list->prev;

		list->prev->next = next;
		list->prev = next;
	}

	return list;
}
dst_proc_list		*dst_append_func_list(dst_proc_list *list, dst_proc *value, dsc_memory *mem)
{
	if (list == NULL)
	{
		list = (dst_proc_list *)dsc_alloc(sizeof(dst_proc_list), mem);

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_proc_list *next = (dst_proc_list *)dsc_alloc(sizeof(dst_proc_list), mem);

		if (next == NULL)
			return NULL;

		next->value = value;
		next->next = list;
		next->prev = list->prev;

		list->prev->next = next;
		list->prev = next;
	}

	return list;
}

size_t dst_exp_list_count(dst_exp_list *list)
{
	if (list == NULL)
		return 0;

	dst_exp_list *current = list->next;

	size_t count = 1;
	while (current != list)
	{
		++count;
		current = current->next;
	}
	return count;
}
size_t dst_type_list_count(dst_type_list *list)
{
	if (list == NULL)
		return 0;

	dst_type_list *current = list->next;

	size_t count = 1;
	while (current != list)
	{
		++count;
		current = current->next;
	}
	return count;
}
size_t dst_proc_param_list_count(dst_proc_param_list *list)
{
	if (list == NULL)
		return 0;

	dst_proc_param_list *current = list->next;

	size_t count = 1;
	while (current != list)
	{
		++count;
		current = current->next;
	}
	return count;
}

int dst_type_list_is_integer(dst_type_list *list)
{
	return (list->value == dst_type_integer) && (list->next == list);
}
int dst_type_list_is_real(dst_type_list *list)
{
	return (list->value == dst_type_real) && (list->next == list);
}
int dst_type_list_is_composite(dst_type_list *list)
{
	return list->next != list;
}