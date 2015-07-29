#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

/* Constructors, Destructors, Accessors */

ast_statement *ast_create_statement_definition(ast_id_list *variables, ast_exp_list *assignments)
{
	ast_statement *statement = (ast_statement *)malloc(sizeof(ast_statement));

	if (statement == NULL)
		return NULL;

	statement->type = ast_statement_type_definition;
	statement->definition.variables = variables;
	statement->definition.values = assignments;

	return statement;
}
ast_statement *ast_create_statement_assignment(ast_id_list *variables, ast_exp_list *assignments)
{
	ast_statement *statement = (ast_statement *)malloc(sizeof(ast_statement));

	if (statement == NULL)
		return NULL;

	statement->type = ast_statement_type_assignment;
	statement->assignment.variables = variables;
	statement->assignment.values = assignments;

	return statement;
}
ast_statement *ast_create_statement_call(char *function, ast_exp_list *parameters)
{
	ast_statement *statement = (ast_statement *)malloc(sizeof(ast_statement));

	if (statement == NULL)
		return NULL;

	statement->type = ast_statement_type_call;
	statement->call.function = function;
	statement->call.parameters = parameters;

	return statement;
}
ast_statement *ast_create_statement_block(ast_statement_list *statements)
{
	ast_statement *statement = (ast_statement *)malloc(sizeof(ast_statement));

	if (statement == NULL)
		return NULL;

	statement->type = ast_statement_type_block;
	statement->block.statements = statements;

	return statement;
}
ast_statement *ast_create_statement_if(ast_exp *condition, ast_statement *true_statement, ast_statement *false_statement)
{
	if (condition == NULL || true_statement == NULL || false_statement == NULL)
		return NULL;

	ast_statement *statement = (ast_statement *)malloc(sizeof(ast_statement));

	if (statement == NULL)
		return NULL;

	statement->type = ast_statement_type_if;
	statement->if_else.condition = condition;
	statement->if_else.true_statement = true_statement;
	statement->if_else.false_statement = false_statement;

	return statement;
}
ast_statement *ast_create_statement_while(ast_exp *condition, ast_statement *loop_statement)
{
	if (condition == NULL || loop_statement == NULL)
		return NULL;

	ast_statement *statement = (ast_statement *)malloc(sizeof(ast_statement));

	if (statement == NULL)
		return NULL;

	statement->type = ast_statement_type_while;
	statement->while_loop.condition = condition;
	statement->while_loop.loop_statement = loop_statement;

	return statement;
}
ast_statement *ast_create_statement_return(ast_exp_list *values)
{
	if (values == NULL)
		return NULL;

	ast_statement *statement = (ast_statement *)malloc(sizeof(ast_statement));

	if (statement == NULL)
		return NULL;

	statement->type = ast_statement_type_return;
	statement->ret.values = values;

	return statement;
}

ast_exp *ast_create_exp_var(char *value)
{
	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_variable;
	exp->variable.id = value;
	exp->temp_count_est = 0;

	return exp;
}
ast_exp *ast_create_exp_int(int value)
{
	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_integer;
	exp->integer.value = value;
	exp->temp_count_est = 1;

	return exp;
}
ast_exp *ast_create_exp_real(float value)
{
	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_real;
	exp->real.value = value;
	exp->temp_count_est = 1;

	return exp;
}
ast_exp *ast_create_exp_cast(ast_type dest_type, ast_exp *value)
{
	if (value == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_cast;
	exp->cast.dest_type = dest_type;
	exp->cast.value = value;
	exp->temp_count_est = max(value->temp_count_est, 1);

	return exp;
}
ast_exp *ast_create_exp_add(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_addition;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_sub(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_subtraction;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_mul(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_multiplication;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_div(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_division;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_cmp_l(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_less;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_cmp_le(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_less_eq;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_cmp_g(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_greater;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_cmp_ge(ast_exp *left, ast_exp *right)
{
	if (left == NULL || right == NULL)
		return NULL;

	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_greater_eq;
	exp->operator.left = left;
	exp->operator.right = right;
	exp->temp_count_est = max(left->temp_count_est, right->temp_count_est) +
		(left->temp_count_est == right->temp_count_est) ? 1 : 0;

	return exp;
}
ast_exp *ast_create_exp_call(char *function, ast_exp_list *parameters)
{
	ast_exp *exp = (ast_exp *)malloc(sizeof(ast_exp));

	if (exp == NULL)
		return NULL;

	exp->type = ast_exp_type_call;
	exp->call.function = function;
	exp->call.parameters = parameters;
	exp->temp_count_est = max(ast_exp_list_count(parameters), 1);

	return exp;
}

ast_func_param	*ast_create_func_param(char *id, ast_type type)
{
	ast_func_param *func_param = (ast_func_param *)malloc(sizeof(ast_func_param));

	if (func_param == NULL)
		return NULL;
	
	func_param->id = id;
	func_param->type = type;

	return func_param;
}
ast_func *ast_create_func(char *id, ast_func_param_list *in_params, ast_type_list *out_types, ast_statement *code)
{
	ast_func *func = (ast_func *)malloc(sizeof(ast_func));

	if (func == NULL)
		return NULL;

	func->id = id;
	func->in_params = in_params;
	func->out_types = out_types;
	func->statement = code;

	return func;
}

void ast_destroy_statement(ast_statement *statement)
{
	if (statement == NULL)
		return;

	switch (statement->type)
	{
	case ast_statement_type_definition:
		ast_destroy_exp_list(statement->definition.values);
		ast_destroy_id_list(statement->definition.variables);
		break;

	case ast_statement_type_assignment:
		ast_destroy_exp_list(statement->assignment.values);
		ast_destroy_id_list(statement->assignment.variables);
		break;

	case ast_statement_type_call:
		ast_destroy_exp_list(statement->call.parameters);
		free(statement->call.function);
		break;

	case ast_statement_type_block:
		ast_destroy_statement_list(statement->block.statements);
		break;
	case ast_statement_type_if:
		ast_destroy_exp(statement->if_else.condition);
		ast_destroy_statement(statement->if_else.true_statement);
		ast_destroy_statement(statement->if_else.false_statement);
		break;
	case ast_statement_type_while:
		ast_destroy_exp(statement->while_loop.condition);
		ast_destroy_statement(statement->while_loop.loop_statement);
		break;
	}

	free(statement);
}
void ast_destroy_exp(ast_exp *exp)
{
	if (exp == NULL)
		return;

	switch (exp->type)
	{
	case ast_exp_type_variable:
		free(exp->variable.id);
		break;
	case ast_exp_type_integer:
	case ast_exp_type_real:
		break;
	case ast_exp_type_cast:
		ast_destroy_exp(exp->cast.value);
		break;
	case ast_exp_type_addition:
	case ast_exp_type_subtraction:
	case ast_exp_type_multiplication:
	case ast_exp_type_division:
	case ast_exp_type_less:
	case ast_exp_type_less_eq:
	case ast_exp_type_greater:
	case ast_exp_type_greater_eq:
		ast_destroy_exp(exp->operator.left);
		ast_destroy_exp(exp->operator.right);
		break;

	case ast_exp_type_call:
		ast_destroy_exp_list(exp->call.parameters);
		free(exp->call.function);
		break;
	}

	free(exp);
}
void ast_destroy_func_param(ast_func_param *func_param)
{
	if (func_param == NULL)
		return;

	free(func_param->id);
	free(func_param);
}
void ast_destroy_func(ast_func *func)
{
	if (func == NULL)
		return;

	free(func->id);
	ast_destroy_func_param_list(func->in_params);
	ast_destroy_type_list(func->out_types);
	ast_destroy_statement(func->statement);

	free(func);
}

ast_type_list		*ast_append_type_list(ast_type_list *list, ast_type value)
{
	if (list == NULL)
	{
		list = (ast_type_list *)malloc(sizeof(ast_type_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		ast_type_list *next = (ast_type_list *)malloc(sizeof(ast_type_list));

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
ast_id_list			*ast_append_id_list(ast_id_list *list, char *value)
{
	if (list == NULL)
	{
		list = (ast_id_list *)malloc(sizeof(ast_id_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		ast_id_list *next = (ast_id_list *)malloc(sizeof(ast_id_list));

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
ast_statement_list	*ast_append_statement_list(ast_statement_list *list, ast_statement *value)
{
	if (list == NULL)
	{
		list = (ast_statement_list *)malloc(sizeof(ast_statement_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		ast_statement_list *next = (ast_statement_list *)malloc(sizeof(ast_statement_list));

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
ast_exp_list		*ast_append_exp_list(ast_exp_list *list, ast_exp *value)
{
	if (list == NULL)
	{
		list = (ast_exp_list *)malloc(sizeof(ast_exp_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		ast_exp_list *next = (ast_exp_list *)malloc(sizeof(ast_exp_list));

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
ast_func_param_list	*ast_append_func_param_list(ast_func_param_list *list, ast_func_param *value)
{
	if (list == NULL)
	{
		list = (ast_func_param_list *)malloc(sizeof(ast_func_param_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		ast_func_param_list *next = (ast_func_param_list *)malloc(sizeof(ast_func_param_list));

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
ast_func_list		*ast_append_func_list(ast_func_list *list, ast_func *value)
{
	if (list == NULL)
	{
		list = (ast_func_list *)malloc(sizeof(ast_func_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		ast_func_list *next = (ast_func_list *)malloc(sizeof(ast_func_list));

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

void ast_destroy_type_list(ast_type_list *list)
{
	ast_type_list *current = list;
	ast_type_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void ast_destroy_id_list(ast_id_list *list)
{
	ast_id_list *current = list;
	ast_id_list *next = NULL;

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
void ast_destroy_statement_list(ast_statement_list *list)
{
	ast_statement_list *current = list;
	ast_statement_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		ast_destroy_statement(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void ast_destroy_exp_list(ast_exp_list *list)
{
	ast_exp_list *current = list;
	ast_exp_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		ast_destroy_exp(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void ast_destroy_func_param_list(ast_func_param_list *list)
{
	ast_func_param_list *current = list;
	ast_func_param_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		ast_destroy_func_param(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}
void ast_destroy_func_list(ast_func_list *list)
{
	ast_func_list *current = list;
	ast_func_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		ast_destroy_func(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}

size_t ast_exp_list_count(ast_exp_list *list)
{
	if (list == NULL)
		return 0;

	ast_exp_list *current = list->next;

	size_t count = 1;
	while (current != list)
	{
		++count;
		current = current->next;
	}
	return count;
}
size_t ast_type_list_count(ast_type_list *list)
{
	if (list == NULL)
		return 0;

	ast_type_list *current = list->next;

	size_t count = 1;
	while (current != list)
	{
		++count;
		current = current->next;
	}
	return count;
}
size_t ast_func_param_list_count(ast_func_param_list *list)
{
	if (list == NULL)
		return 0;

	ast_func_param_list *current = list->next;

	size_t count = 1;
	while (current != list)
	{
		++count;
		current = current->next;
	}
	return count;
}

/* Printing */

void ast_print_type(ast_type type, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (type)
	{
	case ast_type_real:
		printf("{real}");
		break;
	case ast_type_integer:
		printf("{integer}");
		break;
	default:
		printf("{}");
		break;
	}
}
void ast_print_id(char *id, int tab_level)
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
void ast_print_statement(ast_statement *statement, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (statement->type)
	{
	case ast_statement_type_definition:
		printf("variables definition\n");
		ast_print_id_list(statement->definition.variables, tab_level + 1);
		printf("\n\n");
		ast_print_exp_list(statement->definition.values, tab_level + 1);
		break;

	case ast_statement_type_assignment:
		printf("variables assignment\n");
		ast_print_id_list(statement->assignment.variables, tab_level + 1);
		printf("\n\n");
		ast_print_exp_list(statement->assignment.values, tab_level + 1);
		break;

	case ast_statement_type_call:
		printf("call ");
		ast_print_id(statement->call.function, 0);
		printf("\n");
		ast_print_exp_list(statement->call.parameters, tab_level + 1);
		break;

	case ast_statement_type_block:
		printf("block\n");
		ast_print_statement_list(statement->block.statements, tab_level + 1);
		break;

	case ast_statement_type_if:
		printf("if\n");
		ast_print_exp(statement->if_else.condition, tab_level + 1);
		printf("\n\n");
		ast_print_statement(statement->if_else.true_statement, tab_level + 1);
		printf("\n\n");
		ast_print_statement(statement->if_else.false_statement, tab_level + 1);
		break;

	case ast_statement_type_while:
		printf("while\n");
		ast_print_exp(statement->while_loop.condition, tab_level + 1);
		printf("\n\n");
		ast_print_statement(statement->while_loop.loop_statement, tab_level + 1);
		break;

	default:
		printf("{unknown statement}\n");
		break;
	}
}
void ast_print_exp(ast_exp *exp, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (exp->type)
	{
	case ast_exp_type_variable:
		printf("variable (");
		ast_print_id(exp->variable.id, 0);
		printf(")");
		break;

	case ast_exp_type_integer:
		printf("integer (%d)", exp->integer.value);
		break;

	case ast_exp_type_real:
		printf("real (%f)", exp->real.value);
		break;

	case ast_exp_type_cast:
		printf("cast to ");
		ast_print_type(exp->cast.dest_type, 0);
		printf("\n");
		ast_print_exp(exp->cast.value, tab_level + 1);
		break;

	case ast_exp_type_addition:
		printf("addition\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;

	case ast_exp_type_subtraction:
		printf("subtraction\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;

	case ast_exp_type_multiplication:
		printf("multiplication\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;

	case ast_exp_type_division:
		printf("division\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;
	
	case ast_exp_type_less:
		printf("less\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;

	case ast_exp_type_less_eq:
		printf("less eq\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;

	case ast_exp_type_greater:
		printf("greater\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;

	case ast_exp_type_greater_eq:
		printf("greater eq\n");
		ast_print_exp(exp->operator.left, tab_level + 1);
		printf("\n");
		ast_print_exp(exp->operator.right, tab_level + 1);
		break;

	case ast_exp_type_call:
		printf("call ");
		ast_print_id(exp->call.function, 0);
		printf("\n");
		ast_print_exp_list(exp->call.parameters, tab_level + 1);
		break;

	default:
		printf("{unknown expression}\n");
		break;
	}
}
void ast_print_func_param(ast_func_param *function_param, int tab_level)
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
		ast_print_id(function_param->id, 0);
		printf(" : ");
		ast_print_type(function_param->type, 0);
	}
}
void ast_print_func(ast_func *function)
{
	if (function == NULL)
	{
		printf("{null}");
	}
	else
	{
		printf("function ");
		ast_print_id(function->id, 0);
		printf("\n");
		
		printf("in params\n");
		ast_print_func_param_list(function->in_params, 1);
		printf("\n");
		
		printf("out types\n");
		ast_print_type_list(function->out_types, 1);
		printf("\n");

		printf("code\n");
		ast_print_statement(function->statement, 1);
	}
}

void ast_print_type_list(ast_type_list *list, int tab_level)
{
	ast_type_list *current = list;

	while (current != NULL)
	{
		ast_print_type(current->value, tab_level);

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
void ast_print_id_list(ast_id_list *list, int tab_level)
{
	ast_id_list *current = list;

	while (current != NULL)
	{
		ast_print_id(current->value, tab_level);

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
void ast_print_statement_list(ast_statement_list *list, int tab_level)
{
	ast_statement_list *current = list;

	while (current != NULL)
	{
		ast_print_statement(current->value, tab_level);

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
void ast_print_exp_list(ast_exp_list *list, int tab_level)
{
	ast_exp_list *current = list;

	while (current != NULL)
	{
		ast_print_exp(current->value, tab_level);

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
void ast_print_func_param_list(ast_func_param_list *list, int tab_level)
{
	ast_func_param_list *current = list;

	while (current != NULL)
	{
		ast_print_func_param(current->value, tab_level);

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
void ast_print_func_list(ast_func_list *list)
{
	ast_func_list *current = list;

	while (current != NULL)
	{
		ast_print_func(current->value);

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