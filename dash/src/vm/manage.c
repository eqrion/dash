#include "../vm_internal.h"
#include "../hash.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// interface

int		dvm_create_context(dvm_context **context, size_t initial_function_capacity, size_t initial_bytecode_capacity)
{
	dvm_context *result = (dvm_context *)malloc(sizeof(dvm_context));

	if (result == NULL)
		return 0;

	result->function_capacity = initial_function_capacity;
	result->function_count = 0;
	result->function = (dvm_procedure *)malloc(sizeof(dvm_procedure) * initial_function_capacity);

	if (result->function == NULL)
	{
		dvm_destroy_context(result);
		return 0;
	}

	result->bytecode_capacity = initial_bytecode_capacity;
	result->bytecode_count = 0;
	result->bytecode = malloc(sizeof(dvm_bc) * initial_bytecode_capacity);

	if (result->bytecode == NULL)
	{
		dvm_destroy_context(result);
		return 0;
	}

	*context = result;

	return 1;
}
void	dvm_destroy_context(dvm_context *context)
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

dvm_procedure	*dvm_find_proc(const char *name, dvm_context *context)
{
	uint32_t hashed_name = dsh_hash(name);

	for (size_t i = 0; i < context->function_count; ++i)
	{
		if (context->function[i].hashed_name == hashed_name)
			return &context->function[i];
	}

	return NULL;
}
void			 dvm_dissasm_proc(dvm_procedure *function, FILE *out, dvm_context *context)
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
		dvm_bc bc = context->bytecode[cur_pc];

		switch (bc.opcode)
		{
		case dvm_opcode_nop:
			fprintf(out, "nop\n");
			break;

		case dvm_opcode_call:
			fprintf(out, "call  func[%u] i%u o%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_ret:
			fprintf(out, "ret   o%u\n", bc.a);
			break;

		case dvm_opcode_mov:
			fprintf(out, "mov   r%u -> r%u\n", bc.a, bc.c);
			break;

		case dvm_opcode_stor:
			++cur_pc;
			fprintf(out, "stor  %i or %f -> r%u\n",
				*(int32_t *)(&context->bytecode[cur_pc]),
				*(float *)(&context->bytecode[cur_pc]),
				bc.c);
			break;

		case dvm_opcode_cmpi_l:
			fprintf(out, "cmpi  r%u < r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_cmpf_l:
			fprintf(out, "cmpf  r%u < r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_cmpi_le:
			fprintf(out, "cmpi  r%u <= r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_cmpf_le:
			fprintf(out, "cmpf  r%u <= r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_jmp_c:
		{
			uint8_t offset = bc.c;

			fprintf(out, "jmpc  r%u %i\n", bc.a, *(int8_t *)&offset);
			break;
		}

		case dvm_opcode_jmp_cn:
		{
			uint8_t offset = bc.c;

			fprintf(out, "jmpcn r%u %i\n", bc.a, *(int8_t *)&offset);
			break;
		}

		case dvm_opcode_jmp_u:
		{
			uint8_t offset = bc.c;

			fprintf(out, "jmpu  %i\n", *(int8_t *)&offset);
			break;
		}

		case dvm_opcode_addi:
			fprintf(out, "addi  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_addf:
			fprintf(out, "addf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_subi:
			fprintf(out, "subi  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_subf:
			fprintf(out, "subf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_muli:
			fprintf(out, "muli  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_mulf:
			fprintf(out, "mulf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_divi:
			fprintf(out, "divi  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_divf:
			fprintf(out, "divf  r%u, r%u -> r%u\n", bc.a, bc.b, bc.c);
			break;

		case dvm_opcode_casti:
			fprintf(out, "casti r%u -> r%u\n", bc.a, bc.c);
			break;

		case dvm_opcode_castf:
			fprintf(out, "castf r%u -> r%u\n", bc.a, bc.c);
			break;

		default:
			fprintf(out, "unknown instruction\n");
			break;
		}

		++cur_pc;
	}
}

// memory management

dvm_bc			*dvm_context_push_bytecode(size_t amount, dvm_context *context)
{
	if (context == NULL || context->bytecode == NULL)
	{
		return NULL;
	}

	if (context->bytecode_count + amount > context->bytecode_capacity)
	{
		size_t new_bc_capacity = context->bytecode_count + (amount * 2);

		dvm_bc *new_bc = (dvm_bc *)malloc(sizeof(dvm_bc) * new_bc_capacity);

		if (new_bc == NULL)
		{
			return NULL;
		}

		memcpy(new_bc, context->bytecode, sizeof(dvm_bc) * context->bytecode_count);

		context->bytecode_capacity = new_bc_capacity;
		context->bytecode = new_bc;
	}

	dvm_bc *old_top = context->bytecode + context->bytecode_count;
	context->bytecode_count += amount;

	memset(old_top, 0, sizeof(dvm_bc) * amount);

	return old_top;
}
dvm_procedure	*dvm_context_push_procedure(size_t amount, dvm_context *context)
{
	if (context == NULL || context->function == NULL)
	{
		return NULL;
	}

	if (context->function_count + amount > context->function_capacity)
	{
		size_t new_func_capacity = context->function_count + (amount * 2);

		dvm_procedure *new_func = (dvm_procedure *)malloc(sizeof(dvm_procedure) * new_func_capacity);

		if (new_func == NULL)
		{
			return NULL;
		}

		memcpy(new_func, context->function, sizeof(dvm_procedure) * context->function_count);

		context->function_capacity = new_func_capacity;
		context->function = new_func;
	}

	dvm_procedure *old_top = context->function + context->function_count;
	context->function_count += amount;

	memset(old_top, 0, sizeof(dvm_procedure) * amount);

	return old_top;
}

void dvm_context_pop_bytecode(size_t amount, dvm_context *context)
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
void dvm_context_pop_procedure(size_t amount, dvm_context *context)
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

int dvm_context_validate_proc(uint32_t code_start, uint32_t code_length, uint8_t reg_count_in, uint8_t reg_count_use, uint8_t reg_count_out, dvm_context *context)
{
	return 1;
}

// proc gen

int  dvm_proc_emitter_begin_create(dvm_procedure_emitter *procgen, dvm_context *context)
{
	procgen->bytecode_start = context->bytecode_count;
	procgen->bytecode_allocated = 0;
	procgen->context = context;

	return 1;
}

dvm_bc *dvm_proc_emitter_push_bc(size_t amount, dvm_procedure_emitter *procgen)
{
	dvm_bc *result = dvm_context_push_bytecode(amount, procgen->context);

	if (result)
	{
		procgen->bytecode_allocated += amount;
	}

	return result;
}

dvm_procedure	*dvm_proc_emitter_finalize(const char *name, uint8_t reg_count_in, uint8_t reg_count_use, uint8_t reg_count_out, dvm_procedure_emitter *procgen)
{
	if (!dvm_context_validate_proc(procgen->bytecode_start, procgen->bytecode_allocated, reg_count_in, reg_count_use, reg_count_out, procgen->context))
	{
		return NULL;
	}

	dvm_procedure *proc = dvm_context_push_procedure(1, procgen->context);

	if (proc == NULL)
	{
		return NULL;
	}

	proc->c_function = NULL;
	proc->hashed_name = dsh_hash(name);
	proc->reg_count_in = reg_count_in;
	proc->reg_count_use = reg_count_use;
	proc->reg_count_out = reg_count_out;
	proc->bytecode_start = procgen->bytecode_start;
	proc->bytecode_end = procgen->bytecode_start + procgen->bytecode_allocated;

	return proc;
}
void			 dvm_proc_emitter_cancel(dvm_procedure_emitter *procgen)
{
	dvm_context_pop_bytecode(procgen->bytecode_allocated, procgen->context);
}