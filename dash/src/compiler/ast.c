#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

/* Constructors, Destructors, Accessors */

dst_statement *dst_create_statement_definition(dst_id_list *variables, dst_exp_list *assignments)
{
	dst_statement *statement = (dst_statement *)malloc(sizeof(dst_statement));

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_definition;
	statement->definition.variables = variables;
	statement->definition.values = assignments;

	return statement;
}
dst_statement *dst_create_statement_assignment(dst_id_list *variables, dst_exp_list *assignments)
{
	dst_statement *statement = (dst_statement *)malloc(sizeof(dst_statement));

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_assignment;
	statement->assignment.variables = variables;
	statement->assignment.values = assignments;

	return statement;
}
dst_statement *dst_create_statement_call(char *function, dst_exp_list *parameters)
{
	dst_statement *statement = (dst_statement *)malloc(sizeof(dst_statement));

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_call;
	statement->call.function = function;
	statement->call.parameters = parameters;

	return statement;
}
dst_statement *dst_create_statement_block(dst_statement_list *statements)
{
	dst_statement *statement = (dst_statement *)malloc(sizeof(dst_statement));

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_block;
	statement->block.statements = statements;

	return statement;
}
dst_statement *dst_create_statement_if(dst_exp *condition, dst_statement *true_statement, dst_statement *false_statement)
{
	if (condition == NULL || true_statement == NULL || false_statement == NULL)
		return NULL;

	dst_statement *statement = (dst_statement *)malloc(sizeof(dst_statement));

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_if;
	statement->if_else.condition = condition;
	statement->if_else.true_statement = true_statement;
	statement->if_else.false_statement = false_statement;

	return statement;
}
dst_statement *dst_create_statement_while(dst_exp *condition, dst_statement *loop_statement)
{
	if (condition == NULL || loop_statement == NULL)
		return NULL;

	dst_statement *statement = (dst_statement *)malloc(sizeof(dst_statement));

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_while;
	statement->while_loop.condition = condition;
	statement->while_loop.loop_statement = loop_statement;

	return statement;
}
dst_statement *dst_create_statement_return(dst_exp_list *values)
{
	if (values == NULL)
		return NULL;

	dst_statement *statement = (dst_statement *)malloc(sizeof(dst_statement));

	if (statement == NULL)
		return NULL;

	statement->type = dst_statement_type_return;
	statement->ret.values = values;

	return statement;
}

dst_exp *dst_create_exp_var(char *value)
{
	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_variable;
	exp->variable.id = value;
	exp->temp_count_est = 0;

	return exp;
}
dst_exp *dst_create_exp_int(int value)
{
	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_integer;
	exp->integer.value = value;
	exp->temp_count_est = 1;

	return exp;
}
dst_exp *dst_create_exp_real(float value)
{
	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_real;
	exp->real.value = value;
	exp->temp_count_est = 1;

	return exp;
}
dst_exp *dst_create_exp_cast(dst_type dest_type, dst_exp *value)
{
	if (value == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_cast;
	exp->cast.dest_type = dest_type;
	exp->cast.value = value;
	exp->temp_count_est = max(value->temp_count_est, 1);

	return exp;
}
dst_exp *dst_create_exp_add(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_addition;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_sub(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_subtraction;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_mul(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_multiplication;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_div(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_division;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_cmp_l(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_less;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_cmp_le(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_less_eq;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_cmp_g(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_greater;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_cmp_ge(dst_exp *left, dst_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_greater_eq;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		((left->temp_count_est == right->temp_count_est) ? 1 : 0);

	return exp;
}
dst_exp *dst_create_exp_call(char *function, dst_exp_list *parameters)
{
	dst_exp *exp = (dst_exp *)malloc(sizeof(dst_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dst_exp_type_call;
	exp->call.function = function;
	exp->call.parameters = parameters;
	exp->temp_count_est = max(dst_exp_list_count(parameters), 1);

	return exp;
}

dst_proc_param	*dst_create_func_param(char *id, dst_type type)
{
	dst_proc_param *func_param = (dst_proc_param *)malloc(sizeof(dst_proc_param));

	if (func_param == NULL)
		return NULL;
	
	func_param->id = id;
	func_param->type = type;

	return func_param;
}
dst_proc *dst_create_func(char *id, dst_proc_param_list *in_params, dst_type_list *out_types, dst_statement *code)
{
	dst_proc *func = (dst_proc *)malloc(sizeof(dst_proc));

	if (func == NULL)
		return NULL;

	func->id = id;
	func->in_params = in_params;
	func->out_types = out_types;
	func->statement = code;

	return func;
}

void dst_destroy_statement(dst_statement *statement)
{
	if (statement == NULL)
		return;

	switch (statement->type)
	{
	case dst_statement_type_definition:
		dst_destroy_exp_list(statement->definition.values);
		dst_destroy_id_list(statement->definition.variables);
		break;

	case dst_statement_type_assignment:
		dst_destroy_exp_list(statement->assignment.values);
		dst_destroy_id_list(statement->assignment.variables);
		break;

	case dst_statement_type_call:
		dst_destroy_exp_list(statement->call.parameters);
		free(statement->call.function);
		break;

	case dst_statement_type_block:
		dst_destroy_statement_list(statement->block.statements);
		break;
	case dst_statement_type_if:
		dst_destroy_exp(statement->if_else.condition);
		dst_destroy_statement(statement->if_else.true_statement);
		dst_destroy_statement(statement->if_else.false_statement);
		break;
	case dst_statement_type_while:
		dst_destroy_exp(statement->while_loop.condition);
		dst_destroy_statement(statement->while_loop.loop_statement);
		break;
	}

	free(statement);
}
void dst_destroy_exp(dst_exp *exp)
{
	if (exp == NULL)
		return;

	switch (exp->type)
	{
	case dst_exp_type_variable:
		free(exp->variable.id);
		break;
	case dst_exp_type_integer:
	case dst_exp_type_real:
		break;
	case dst_exp_type_cast:
		dst_destroy_exp(exp->cast.value);
		break;
	case dst_exp_type_addition:
	case dst_exp_type_subtraction:
	case dst_exp_type_multiplication:
	case dst_exp_type_division:
	case dst_exp_type_less:
	case dst_exp_type_less_eq:
	case dst_exp_type_greater:
	case dst_exp_type_greater_eq:
		dst_destroy_exp(exp->operator.left);
		dst_destroy_exp(exp->operator.right);
		break;

	case dst_exp_type_call:
		dst_destroy_exp_list(exp->call.parameters);
		free(exp->call.function);
		break;
	}

	free(exp);
}
void dst_destroy_func_param(dst_proc_param *func_param)
{
	if (func_param == NULL)
		return;

	free(func_param->id);
	free(func_param);
}
void dst_destroy_func(dst_proc *func)
{
	if (func == NULL)
		return;

	free(func->id);
	dst_destroy_func_param_list(func->in_params);
	dst_destroy_type_list(func->out_types);
	dst_destroy_statement(func->statement);

	free(func);
}

dst_type_list		*dst_append_type_list(dst_type_list *list, dst_type value)
{
	if (list == NULL)
	{
		list = (dst_type_list *)malloc(sizeof(dst_type_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_type_list *next = (dst_type_list *)malloc(sizeof(dst_type_list));

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
dst_id_list			*dst_append_id_list(dst_id_list *list, char *value)
{
	if (list == NULL)
	{
		list = (dst_id_list *)malloc(sizeof(dst_id_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_id_list *next = (dst_id_list *)malloc(sizeof(dst_id_list));

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
dst_statement_list	*dst_append_statement_list(dst_statement_list *list, dst_statement *value)
{
	if (list == NULL)
	{
		list = (dst_statement_list *)malloc(sizeof(dst_statement_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_statement_list *next = (dst_statement_list *)malloc(sizeof(dst_statement_list));

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
dst_exp_list		*dst_append_exp_list(dst_exp_list *list, dst_exp *value)
{
	if (list == NULL)
	{
		list = (dst_exp_list *)malloc(sizeof(dst_exp_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_exp_list *next = (dst_exp_list *)malloc(sizeof(dst_exp_list));

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
dst_proc_param_list	*dst_append_func_param_list(dst_proc_param_list *list, dst_proc_param *value)
{
	if (list == NULL)
	{
		list = (dst_proc_param_list *)malloc(sizeof(dst_proc_param_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_proc_param_list *next = (dst_proc_param_list *)malloc(sizeof(dst_proc_param_list));

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
dst_proc_list		*dst_append_func_list(dst_proc_list *list, dst_proc *value)
{
	if (list == NULL)
	{
		list = (dst_proc_list *)malloc(sizeof(dst_proc_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dst_proc_list *next = (dst_proc_list *)malloc(sizeof(dst_proc_list));

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

void dst_destroy_type_list(dst_type_list *list)
{
	dst_type_list *current = list;
	dst_type_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void dst_destroy_id_list(dst_id_list *list)
{
	dst_id_list *current = list;
	dst_id_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		free(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void dst_destroy_statement_list(dst_statement_list *list)
{
	dst_statement_list *current = list;
	dst_statement_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		dst_destroy_statement(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void dst_destroy_exp_list(dst_exp_list *list)
{
	dst_exp_list *current = list;
	dst_exp_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		dst_destroy_exp(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void dst_destroy_func_param_list(dst_proc_param_list *list)
{
	dst_proc_param_list *current = list;
	dst_proc_param_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		dst_destroy_func_param(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void dst_destroy_func_list(dst_proc_list *list)
{
	dst_proc_list *current = list;
	dst_proc_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		dst_destroy_func(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
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

/* Printing */

void dst_print_type(dst_type type, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (type)
	{
	case dst_type_real:
		printf("{real}");
		break;
	case dst_type_integer:
		printf("{integer}");
		break;
	default:
		printf("{}");
		break;
	}
}
void dst_print_id(char *id, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	if (id == NULL)
	{
		printf("{null}");
	}
	else
	{
		printf("%s", id);
	}
}
void dst_print_statement(dst_statement *statement, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (statement->type)
	{
	case dst_statement_type_definition:
		printf("variables definition\n");
		dst_print_id_list(statement->definition.variables, tab_level + 1);
		printf("\n\n");
		dst_print_exp_list(statement->definition.values, tab_level + 1);
		break;

	case dst_statement_type_assignment:
		printf("variables assignment\n");
		dst_print_id_list(statement->assignment.variables, tab_level + 1);
		printf("\n\n");
		dst_print_exp_list(statement->assignment.values, tab_level + 1);
		break;

	case dst_statement_type_call:
		printf("call ");
		dst_print_id(statement->call.function, 0);
		printf("\n");
		dst_print_exp_list(statement->call.parameters, tab_level + 1);
		break;

	case dst_statement_type_block:
		printf("block\n");
		dst_print_statement_list(statement->block.statements, tab_level + 1);
		break;

	case dst_statement_type_if:
		printf("if\n");
		dst_print_exp(statement->if_else.condition, tab_level + 1);
		printf("\n\n");
		dst_print_statement(statement->if_else.true_statement, tab_level + 1);
		printf("\n\n");
		dst_print_statement(statement->if_else.false_statement, tab_level + 1);
		break;

	case dst_statement_type_while:
		printf("while\n");
		dst_print_exp(statement->while_loop.condition, tab_level + 1);
		printf("\n\n");
		dst_print_statement(statement->while_loop.loop_statement, tab_level + 1);
		break;

	default:
		printf("{unknown statement}\n");
		break;
	}
}
void dst_print_exp(dst_exp *exp, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (exp->type)
	{
	case dst_exp_type_variable:
		printf("variable (");
		dst_print_id(exp->variable.id, 0);
		printf(")");
		break;

	case dst_exp_type_integer:
		printf("integer (%d)", exp->integer.value);
		break;

	case dst_exp_type_real:
		printf("real (%f)", exp->real.value);
		break;

	case dst_exp_type_cast:
		printf("cast to ");
		dst_print_type(exp->cast.dest_type, 0);
		printf("\n");
		dst_print_exp(exp->cast.value, tab_level + 1);
		break;

	case dst_exp_type_addition:
		printf("addition\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;

	case dst_exp_type_subtraction:
		printf("subtraction\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;

	case dst_exp_type_multiplication:
		printf("multiplication\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;

	case dst_exp_type_division:
		printf("division\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;
	
	case dst_exp_type_less:
		printf("less\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;

	case dst_exp_type_less_eq:
		printf("less eq\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;

	case dst_exp_type_greater:
		printf("greater\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;

	case dst_exp_type_greater_eq:
		printf("greater eq\n");
		dst_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		dst_print_exp(exp->operator.right, tab_level + 1);
		break;

	case dst_exp_type_call:
		printf("call ");
		dst_print_id(exp->call.function, 0);
		printf("\n");
		dst_print_exp_list(exp->call.parameters, tab_level + 1);
		break;

	default:
		printf("{unknown expression}\n");
		break;
	}
}
void dst_print_func_param(dst_proc_param *function_param, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	if (function_param == NULL)
	{
		printf("{null}");
	}
	else
	{
		dst_print_id(function_param->id, 0);
		printf(" : ");
		dst_print_type(function_param->type, 0);
	}
}
void dst_print_func(dst_proc *function)
{
	if (function == NULL)
	{
		printf("{null}");
	}
	else
	{
		printf("function ");
		dst_print_id(function->id, 0);
		printf("\n");
		
		printf("in params\n");
		dst_print_func_param_list(function->in_params, 1);
		printf("\n");
		
		printf("out types\n");
		dst_print_type_list(function->out_types, 1);
		printf("\n");

		printf("code\n");
		dst_print_statement(function->statement, 1);
	}
}

void dst_print_type_list(dst_type_list *list, int tab_level)
{
	dst_type_list *current = list;

	while (current != NULL)
	{
		dst_print_type(current->value, tab_level);

		current = current->next;

		if (current == list)
		{
			break;
		}
		else
		{
			printf("\n");
		}
	}
}
void dst_print_id_list(dst_id_list *list, int tab_level)
{
	dst_id_list *current = list;

	while (current != NULL)
	{
		dst_print_id(current->value, tab_level);

		current = current->next;

		if (current == list)
		{
			break;
		}
		else
		{
			printf("\n");
		}
	}
}
void dst_print_statement_list(dst_statement_list *list, int tab_level)
{
	dst_statement_list *current = list;

	while (current != NULL)
	{
		dst_print_statement(current->value, tab_level);

		current = current->next;

		if (current == list)
		{
			break;
		}
		else
		{
			printf("\n");
		}
	}
}
void dst_print_exp_list(dst_exp_list *list, int tab_level)
{
	dst_exp_list *current = list;

	while (current != NULL)
	{
		dst_print_exp(current->value, tab_level);

		current = current->next;

		if (current == list)
		{
			break;
		}
		else
		{
			printf("\n");
		}
	}
}
void dst_print_func_param_list(dst_proc_param_list *list, int tab_level)
{
	dst_proc_param_list *current = list;

	while (current != NULL)
	{
		dst_print_func_param(current->value, tab_level);

		current = current->next;

		if (current == list)
		{
			break;
		}
		else
		{
			printf("\n");
		}
	}
}
void dst_print_func_list(dst_proc_list *list)
{
	dst_proc_list *current = list;

	while (current != NULL)
	{
		dst_print_func(current->value);

		current = current->next;

		printf("\n");

		if (current == list)
		{
			break;
		}
	}
}