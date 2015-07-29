#include "../context.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int dsh_create_context(dsh_context **context, size_t initial_function_capacity, size_t initial_bytecode_capacity)
{
	dsh_context *result = (dsh_context *)malloc(sizeof(dsh_context));

	if (result == NULL)
		return 0;

	result->function_capacity = initial_function_capacity;
	result->function_count = 0;
	result->function = (dsh_function_def *)malloc(sizeof(dsh_function_def) * initial_function_capacity);

	if (result->function == NULL)
	{
		dsh_destroy_context(result);
		return 0;
	}

	result->bytecode_capacity = initial_bytecode_capacity;
	result->bytecode_count = 0;
	result->bytecode = malloc(sizeof(dsh_bc) * initial_bytecode_capacity);

	if (result->bytecode == NULL)
	{
		dsh_destroy_context(result);
		return 0;
	}

	*context = result;

	return 1;
}
void dsh_destroy_context(dsh_context *context)
{
	if (context->bytecode != NULL)
	{
		free(context->bytecode);
		context->bytecode = NULL;
	}
	if (context->function != NULL)
	{
		free(context->function);
		context->function = NULL;
	}
	free(context);
}

dsh_bc *dsh_context_push_bytecode(size_t amount, dsh_context *context)
{
	if (context == NULL || context->bytecode == NULL)
	{
		return NULL;
	}

	if (context->bytecode_count + amount > context->bytecode_capacity)
	{
		size_t new_bc_capacity = context->bytecode_count + (amount * 2);

		dsh_bc *new_bc = (dsh_bc *)malloc(sizeof(dsh_bc) * new_bc_capacity);

		if (new_bc == NULL)
		{
			return NULL;
		}

		memcpy(new_bc, context->bytecode, sizeof(dsh_bc) * context->bytecode_count);

		context->bytecode_capacity = new_bc_capacity;
		context->bytecode = new_bc;
	}

	dsh_bc *old_top = context->bytecode + context->bytecode_count;
	context->bytecode_count += amount;

	memset(old_top, 0, sizeof(dsh_bc) * amount);

	return old_top;
}
dsh_function_def *dsh_context_push_function(size_t amount, dsh_context *context)
{
	if (context == NULL || context->function == NULL)
	{
		return NULL;
	}

	if (context->function_count + amount > context->function_capacity)
	{
		size_t new_func_capacity = context->function_count + (amount * 2);

		dsh_function_def *new_func = (dsh_function_def *)malloc(sizeof(dsh_function_def) * new_func_capacity);

		if (new_func == NULL)
		{
			return NULL;
		}

		memcpy(new_func, context->function, sizeof(dsh_function_def) * context->function_count);

		context->function_capacity = new_func_capacity;
		context->function = new_func;
	}

	dsh_function_def *old_top = context->function + context->function_count;
	context->function_count += amount;

	memset(old_top, 0, sizeof(dsh_function_def) * amount);

	return old_top;
}

void dsh_context_pop_bytecode(size_t amount, dsh_context *context)
{
	if (context == NULL || context->bytecode == NULL)
	{
		return;
	}

	if (amount <= context->bytecode_count)
	{
		context->bytecode_count -= amount;
	}
	else
	{
		context->bytecode_count = 0;
	}
}
void dsh_context_pop_function(size_t amount, dsh_context *context)
{
	if (context == NULL || context->function == NULL)
	{
		return;
	}

	if (amount <= context->function_count)
	{
		context->function_count -= amount;
	}
	else
	{
		context->function_count = 0;
	}
}

dsh_function_def *dsh_context_find_function(uint32_t hashed_name, dsh_context *context)
{
	for (size_t i = 0; i < context->function_count; ++i)
	{
		if (context->function[i].hashed_name == hashed_name)
			return &context->function[i];
	}

	return NULL;
}
size_t dsh_context_find_function_index(uint32_t hashed_name, dsh_context *context)
{
	for (size_t i = 0; i < context->function_count; ++i)
	{
		if (context->function[i].hashed_name == hashed_name)
			return i;
	}

	return ~0;
}