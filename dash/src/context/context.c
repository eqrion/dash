#include "../context.h"
#include "../hash.h"

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

dsh_function_def *dsh_context_find_func(const char *name, dsh_context *context)
{
	uint32_t hashed_name = dsh_hash(name);

	for (size_t i = 0; i < context->function_count; ++i)
	{
		if (context->function[i].hashed_name == hashed_name)
			return &context->function[i];
	}

	return NULL;
}

void dsh_context_dissasm_func(dsh_function_def *function, FILE *out, dsh_context *context)
{
	if (function->c_function != NULL)
	{
		printf("c-func - in: %u use: %u out: %u\n", function->reg_count_in, function->reg_count_use, function->reg_count_out);
	}
	else
	{
		printf("dsh-func - in: %u use: %u out: %u\n", function->reg_count_in, function->reg_count_use, function->reg_count_out);
	}

	size_t cur_pc = function->bytecode_start;

	while (cur_pc < function->bytecode_end)
	{
		dsh_bc bc = context->bytecode[cur_pc];

		switch (bc.opcode)
		{
			case dsh_opcode_nop:
				fprintf(out, "nop\n");
				break;

			case dsh_opcode_call:
				fprintf(out, "call  func[%u] i%u o%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_ret:
				fprintf(out, "ret   o%u\n", bc.a);
				break;

			case dsh_opcode_mov:
				fprintf(out, "mov   r%u -> r%u\n", bc.a, bc.c);
				break;

			case dsh_opcode_stor:
				++cur_pc;
				fprintf(out, "stor  %i or %f -> r%u\n",
					*(int32_t *)(&context->bytecode[cur_pc]),
					*(float *)(&context->bytecode[cur_pc]),
					bc.c);
				break;

			case dsh_opcode_cmpi_l:
				fprintf(out, "cmpi  r%u < r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_cmpf_l:
				fprintf(out, "cmpf  r%u < r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_cmpi_le:
				fprintf(out, "cmpi  r%u <= r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_cmpf_le:
				fprintf(out, "cmpf  r%u <= r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_jmp_c:
			{
				uint8_t offset = bc.c;

				fprintf(out, "jmpc  r%u %i\n", bc.a, *(int8_t *)&offset);
				break;
			}

			case dsh_opcode_jmp_cn:
			{
				uint8_t offset = bc.c;

				fprintf(out, "jmpcn r%u %i\n", bc.a, *(int8_t *)&offset);
				break;
			}

			case dsh_opcode_jmp_u:
			{
				uint8_t offset = bc.c;

				fprintf(out, "jmpu  %i\n", *(int8_t *)&offset);
				break;
			}

			case dsh_opcode_addi:
				fprintf(out, "addi  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_addf:
				fprintf(out, "addf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_subi:
				fprintf(out, "subi  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_subf:
				fprintf(out, "subf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_muli:
				fprintf(out, "muli  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_mulf:
				fprintf(out, "mulf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_divi:
				fprintf(out, "divi  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_divf:
				fprintf(out, "divf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
				break;

			case dsh_opcode_casti:
				fprintf(out, "casti r%u -> r%u\n", bc.a, bc.c);
				break;

			case dsh_opcode_castf:
				fprintf(out, "castf r%u -> r%u\n", bc.a, bc.c);
				break;

		default:
			fprintf(out, "unknown instruction\n");
			break;
		}

		++cur_pc;
	}
}