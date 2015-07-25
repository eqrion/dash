#include "ast.h"

#include <stdio.h>
#include <stdlib.h>

/* Types */

dsh_type_list *dsh_alloc_type_list(dsh_type_list *list, dsh_type value)
{
	if (list == NULL)
	{
		list = (dsh_type_list *)malloc(sizeof(dsh_type_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dsh_type_list *next = (dsh_type_list *)malloc(sizeof(dsh_type_list));

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
void dsh_dealloc_type_list(dsh_type_list *list)
{
	dsh_type_list *current = list;
	dsh_type_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		free(current);

		current = next;

		if (current == list)
			break;
	}
}

void dsh_print_type(dsh_type type, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (type)
	{
	case dsh_type_real:
		printf("{real}");
		break;
	case dsh_type_integer:
		printf("{integer}");
		break;
	default:
		printf("{}");
		break;
	}
}
void dsh_print_type_list(dsh_type_list *list, int tab_level)
{
	dsh_type_list *current = list;

	while (current != NULL)
	{
		dsh_print_type(current->value, tab_level);

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

/* Identifiers */

dsh_id *dsh_grab_id(char *name)
{
	return name;
}
void dsh_dealloc_id(dsh_id *id)
{
	if (id != NULL)
	{
		free(id);
	}
}

dsh_id_list *dsh_alloc_id_list(dsh_id_list *list, dsh_id *value)
{
	if (list == NULL)
	{
		list = (dsh_id_list *)malloc(sizeof(dsh_id_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dsh_id_list *next = (dsh_id_list *)malloc(sizeof(dsh_id_list));

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
void dsh_dealloc_id_list(dsh_id_list *list)
{
	dsh_id_list *current = list;
	dsh_id_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		dsh_dealloc_id(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}

void dsh_print_id(dsh_id *id, int tab_level)
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
void dsh_print_id_list(dsh_id_list *list, int tab_level)
{
	dsh_id_list *current = list;

	while (current != NULL)
	{
		dsh_print_id(current->value, tab_level);

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

/* Expressions */

dsh_exp *dsh_alloc_exp_var(dsh_id *value)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_variable;
	exp->variable.identifier = value;

	return exp;
}
dsh_exp *dsh_alloc_exp_int(int value)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_integer;
	exp->integer.value = value;

	return exp;
}
dsh_exp *dsh_alloc_exp_real(float value)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_real;
	exp->real.value = value;

	return exp;
}
dsh_exp *dsh_alloc_exp_cast(dsh_type dest_type, dsh_exp *value)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_cast;
	exp->cast.dest_type = dest_type;
	exp->cast.value = value;

	return exp;
}

dsh_exp *dsh_alloc_exp_add(dsh_exp *left, dsh_exp *right)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_addition;
	exp->addition.left = left;
	exp->addition.right = right;

	return exp;
}
dsh_exp *dsh_alloc_exp_sub(dsh_exp *left, dsh_exp *right)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_subtraction;
	exp->subtraction.left = left;
	exp->subtraction.right = right;

	return exp;
}
dsh_exp *dsh_alloc_exp_mul(dsh_exp *left, dsh_exp *right)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_multiplication;
	exp->multiplication.left = left;
	exp->multiplication.right = right;

	return exp;
}
dsh_exp *dsh_alloc_exp_div(dsh_exp *left, dsh_exp *right)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_division;
	exp->division.left = left;
	exp->division.right = right;

	return exp;
}

dsh_exp *dsh_alloc_exp_definition(dsh_id_list *variables, dsh_exp_list *assignments)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_definition;
	exp->definition.variables = variables;
	exp->definition.values = assignments;

	return exp;
}
dsh_exp *dsh_alloc_exp_assignment(dsh_id_list *variables, dsh_exp_list *assignments)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_assignment;
	exp->assignment.variables = variables;
	exp->assignment.values = assignments;

	return exp;
}

dsh_exp *dsh_alloc_exp_call(dsh_id *function, dsh_exp_list *parameters)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_call;
	exp->call.function = function;
	exp->call.parameters = parameters;

	return exp;
}

dsh_exp *dsh_alloc_exp_block(dsh_exp_list *statements)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_block;
	exp->block.statements = statements;

	return exp;
}
dsh_exp *dsh_alloc_exp_if(dsh_exp *condition, dsh_exp_list *true_exp, dsh_exp_list *false_exp)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_if;
	exp->if_else.condition = condition;
	exp->if_else.true_exp = true_exp;
	exp->if_else.false_exp = false_exp;

	return exp;
}
dsh_exp *dsh_alloc_exp_while(dsh_exp *condition, dsh_exp_list *statement)
{
	dsh_exp *exp = (dsh_exp *)malloc(sizeof(dsh_exp));

	if (exp == NULL)
		return NULL;

	exp->type = dsh_exp_type_while;
	exp->while_loop.condition = condition;
	exp->while_loop.exp = statement;

	return exp;
}

void dsh_dealloc_exp(dsh_exp *exp)
{
	if (exp == NULL)
		return;

	switch (exp->type)
	{
		case dsh_exp_type_variable:
			dsh_dealloc_id(exp->variable.identifier);
			break;
		case dsh_exp_type_integer:
		case dsh_exp_type_real:
			break;
		case dsh_exp_type_cast:
			dsh_dealloc_exp(exp->cast.value);
			break;
		case dsh_exp_type_addition:
			dsh_dealloc_exp(exp->addition.left);
			dsh_dealloc_exp(exp->addition.right);
			break;
		case dsh_exp_type_subtraction:
			dsh_dealloc_exp(exp->subtraction.left);
			dsh_dealloc_exp(exp->subtraction.right);
			break;
		case dsh_exp_type_multiplication:
			dsh_dealloc_exp(exp->multiplication.left);
			dsh_dealloc_exp(exp->multiplication.right);
			break;
		case dsh_exp_type_division:
			dsh_dealloc_exp(exp->division.left);
			dsh_dealloc_exp(exp->division.right);
			break;

		case dsh_exp_type_definition:
			dsh_dealloc_exp_list(exp->definition.values);
			dsh_dealloc_id_list(exp->definition.variables);
			break;

		case dsh_exp_type_assignment:
			dsh_dealloc_exp_list(exp->assignment.values);
			dsh_dealloc_id_list(exp->assignment.variables);
			break;

		case dsh_exp_type_call:
			dsh_dealloc_exp_list(exp->call.parameters);
			dsh_dealloc_id(exp->call.function);
			break;

		case dsh_exp_type_block:
			dsh_dealloc_exp_list(exp->block.statements);
			break;
		case dsh_exp_type_if:
			dsh_dealloc_exp(exp->if_else.condition);
			dsh_dealloc_exp_list(exp->if_else.true_exp);
			dsh_dealloc_exp_list(exp->if_else.false_exp);
			break;
		case dsh_exp_type_while:
			dsh_dealloc_exp(exp->while_loop.condition);
			dsh_dealloc_exp_list(exp->while_loop.exp);
			break;
	}

	free(exp);
}

dsh_exp_list *dsh_alloc_exp_list(dsh_exp_list *list, dsh_exp *value)
{
	if (list == NULL)
	{
		list = (dsh_exp_list *)malloc(sizeof(dsh_exp_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dsh_exp_list *next = (dsh_exp_list *)malloc(sizeof(dsh_exp_list));

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
void dsh_dealloc_exp_list(dsh_exp_list *list)
{
	dsh_exp_list *current = list;
	dsh_exp_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		dsh_dealloc_exp(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}

void dsh_print_exp(dsh_exp *exp, int tab_level)
{
	int tabs_left = tab_level;
	while (tabs_left > 0)
	{
		printf("\t");
		--tabs_left;
	}

	switch (exp->type)
	{
	case dsh_exp_type_variable:
		printf("variable (");
		dsh_print_id(exp->variable.identifier, 0);
		printf(")");
		break;

	case dsh_exp_type_integer:
		printf("integer (%d)", exp->integer.value);
		break;

	case dsh_exp_type_real:
		printf("real (%f)", exp->real.value);
		break;

	case dsh_exp_type_cast:
		printf("cast to ");
		dsh_print_type(exp->cast.dest_type, 0);
		printf("\n");
		dsh_print_exp(exp->cast.value, tab_level + 1);
		break;

	case dsh_exp_type_addition:
		printf("addition\n");
		dsh_print_exp(exp->addition.left, tab_level + 1);
		printf("\n");
		dsh_print_exp(exp->addition.right, tab_level + 1);
		break;

	case dsh_exp_type_subtraction:
		printf("subtraction\n");
		dsh_print_exp(exp->subtraction.left, tab_level + 1);
		printf("\n");
		dsh_print_exp(exp->subtraction.right, tab_level + 1);
		break;

	case dsh_exp_type_multiplication:
		printf("multiplication\n");
		dsh_print_exp(exp->multiplication.left, tab_level + 1);
		printf("\n");
		dsh_print_exp(exp->multiplication.right, tab_level + 1);
		break;
	case dsh_exp_type_division:
		printf("division\n");
		dsh_print_exp(exp->division.left, tab_level + 1);
		printf("\n");
		dsh_print_exp(exp->division.right, tab_level + 1);
		break;

	case dsh_exp_type_definition:
		printf("variables definition\n");
		dsh_print_id_list(exp->definition.variables, tab_level + 1);
		printf("\n\n");
		dsh_print_exp_list(exp->assignment.values, tab_level + 1);
		break;

	case dsh_exp_type_assignment:
		printf("variables assignment\n");
		dsh_print_id_list(exp->assignment.variables, tab_level + 1);
		printf("\n\n");
		dsh_print_exp_list(exp->assignment.values, tab_level + 1);
		break;

	case dsh_exp_type_call:
		printf("call ");
		dsh_print_id(exp->call.function, 0);
		printf("\n");
		dsh_print_exp_list(exp->call.parameters, tab_level + 1);
		break;

	case dsh_exp_type_block:
		printf("block\n");
		dsh_print_exp_list(exp->block.statements, tab_level + 1);
		break;

	case dsh_exp_type_if:
		printf("if\n");
		dsh_print_exp(exp->if_else.condition, tab_level + 1);
		printf("\n\n");
		dsh_print_exp_list(exp->if_else.true_exp, tab_level + 1);
		printf("\n\n");
		dsh_print_exp_list(exp->if_else.false_exp, tab_level + 1);
		break;

	case dsh_exp_type_while:
		printf("while\n");
		dsh_print_exp(exp->while_loop.condition, tab_level + 1);
		printf("\n\n");
		dsh_print_exp_list(exp->while_loop.exp, tab_level + 1);
		break;

	default:
		printf("{unknown expression}\n");
		break;
	}
}
void dsh_print_exp_list(dsh_exp_list *list, int tab_level)
{
	dsh_exp_list *current = list;

	while (current != NULL)
	{
		dsh_print_exp(current->value, tab_level);

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

/* Dash Function */

dsh_func *dsh_alloc_func(dsh_id *name, dsh_exp_list *parameters, dsh_type_list *out_types, dsh_exp_list *code)
{
	dsh_func *func = (dsh_func *)malloc(sizeof(dsh_func));

	if (func == NULL)
		return NULL;

	func->name = name;
	func->parameters = parameters;
	func->out_types = out_types;
	func->code = code;
	
	return func;
}
void dsh_dealloc_func(dsh_func *func)
{
	if (func == NULL)
		return;

	dsh_dealloc_id(func->name);
	dsh_dealloc_exp_list(func->parameters);
	dsh_dealloc_type_list(func->out_types);
	dsh_dealloc_exp_list(func->code);

	free(func);
}

dsh_func_list *dsh_alloc_func_list(dsh_func_list *list, dsh_func *value)
{
	if (list == NULL)
	{
		list = (dsh_func_list *)malloc(sizeof(dsh_func_list));

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dsh_func_list *next = (dsh_func_list *)malloc(sizeof(dsh_func_list));

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
void dsh_dealloc_func_list(dsh_func_list *list)
{
	dsh_func_list *current = list;
	dsh_func_list *next = NULL;

	while (current != NULL)
	{
		next = current->next;

		dsh_dealloc_func(current->value);
		free(current);

		current = next;

		if (current == list)
			break;
	}
}

void dsh_print_func(dsh_func *function)
{
	if (function == NULL)
	{
		printf("{null}");
	}
	else
	{
		printf("function ");
		dsh_print_id(function->name, 0);
		printf("\n");
		
		printf("parameters\n");
		dsh_print_exp_list(function->parameters, 1);
		printf("\n");
		
		printf("output types");
		dsh_print_type_list(function->out_types, 1);
		printf("\n");

		printf("code");
		dsh_print_exp_list(function->code, 1);
	}
}
void dsh_print_func_list(dsh_func_list *list)
{
	dsh_func_list *current = list;

	while (current != NULL)
	{
		dsh_print_func(current->value);

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