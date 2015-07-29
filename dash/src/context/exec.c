#include "../context.h"
#include "../hash.h"

#include "stack.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int dsh_context_exec_func(struct dsh_context *context, const char *func_name, const dsh_var *func_parameters, dsh_var *func_results)
{
	uint32_t func_index = (uint32_t)dsh_context_find_function_index(dsh_hash(func_name), context);

	if (func_index >= context->function_count)
	{
		fprintf(stderr, "invalid function.\n");
		return 0;
	}

	// Execution variables 

	uint32_t					cur_func_index = func_index;
	struct dsh_function_def    *cur_func = &context->function[func_index];
	uint32_t					cur_pc = cur_func->bytecode_start;
	uint32_t					cur_frame_size = cur_func->reg_count_in + cur_func->reg_count_use;

	// Allocate the stack for use in executing this function

	struct dsh_stack stack;
	memset(&stack, 0, sizeof(stack));

	if (!dsh_stack_alloc(&stack, 128))
	{
		fprintf(stderr, "error creating a dash stack.\n");
		return 0;
	}
	
	// Push the first function's registers, and parameters over

	if (!dsh_stack_push(&stack, cur_frame_size))
	{
		fprintf(stderr, "stack overflow error.\n");
		return 0;
	}

	memcpy(
		stack.reg_current,
		func_parameters,
		sizeof(dsh_var) * cur_func->reg_count_in
		);

	// Bytecode execution main loop

	while (1)
	{
		dsh_bc instruction = context->bytecode[cur_pc];
		
		switch (instruction.opcode)
		{
		case dsh_opcode_nop:
			break;

		case dsh_opcode_call:
		{
			// Validate the operands:
			//		instruction.a -> call_func_index
			//		instruction.b -> call_in_register_start
			//		instruction.c -> call_out_register_start

			if (instruction.a >= context->function_count)
			{
				fprintf(stderr, "invalid function referenced in call.\n");
				goto execution_error;
			}

			struct dsh_function_def *next_func = &context->function[instruction.a];

			if (instruction.b >= cur_frame_size ||
				(uint32_t)(instruction.b + next_func->reg_count_in) > (uint32_t)(cur_frame_size))
			{
				fprintf(stderr, "invalid register range for function parameters.\n");
				goto execution_error;
			}

			if (instruction.c >= cur_frame_size ||
				(uint32_t)(instruction.c + next_func->reg_count_out) > (uint32_t)(cur_frame_size))
			{
				fprintf(stderr, "invalid register range for return data.\n");
				goto execution_error;
			}

			// Grab the pointer for where to get the parameters, before we push an activation record

			dsh_var *reg_in_start = &stack.reg_current[instruction.b];

			// Push space for an activation record

			if (!dsh_stack_push(&stack, 4))
			{
				fprintf(stderr, "stack overflow error.\n");
				goto execution_error;
			}

			stack.reg_current[0].u = cur_frame_size;
			stack.reg_current[1].u = cur_func_index;
			stack.reg_current[2].u = instruction.c;
			stack.reg_current[3].u = cur_pc;

			// Switch to the new function

			cur_func_index = instruction.a;
			cur_func = next_func;
			cur_pc = next_func->bytecode_start;
			cur_frame_size = next_func->reg_count_in + next_func->reg_count_use;

			if (!dsh_stack_push(&stack, cur_frame_size))
			{
				fprintf(stderr, "stack overflow error.\n");
				goto execution_error;
			}

			// Copy the parameters over

			memcpy(
				stack.reg_current,
				reg_in_start,
				sizeof(dsh_var) * cur_func->reg_count_in
				);

			// Skip over the pc increment and continue on

			continue;
		}
		case dsh_opcode_ret:
		{
			// Validate the operands (instruction.a = reg_out_start)

			if (instruction.a >= cur_frame_size ||
				(uint32_t)(instruction.a + cur_func->reg_count_out) > (uint32_t)(cur_frame_size))
			{
				fprintf(stderr, "invalid register range for result registers.\n");
				goto execution_error;
			}

			// Grab an pointer to the source for return data

			dsh_var *reg_out_start = &stack.reg_current[instruction.a];

			// Pop this function's registers off the stack

			if (!dsh_stack_pop(&stack, cur_frame_size))
			{
				fprintf(stderr, "stack underflow error.\n");
				goto execution_error;
			}

			// If there is no activation record after this, then
			// we have reached the end of the execution.

			if (stack.reg_current == stack.reg_bottom)
			{
				memcpy(
					func_results,
					reg_out_start,
					sizeof(dsh_var) * cur_func->reg_count_out
					);

				goto execution_over;
			}

			// We need to return back to our previous function,
			// grab the activation record from the stack and pop past it

			dsh_var returning_func_frame_size = stack.reg_current[0];
			dsh_var returning_func_index = stack.reg_current[1];
			dsh_var returning_func_result_dest = stack.reg_current[2];
			dsh_var returning_cur_pc = stack.reg_current[3];

			if (!dsh_stack_pop(&stack, 4))
			{
				fprintf(stderr, "stack underflow error.\n");
				goto execution_error;
			}

			// The stack is now back to where it was, now we just
			// need to copy the result, and move the variables back.

			memcpy(
				stack.reg_current + returning_func_result_dest.u,
				reg_out_start,
				sizeof(dsh_var) * cur_func->reg_count_out
				);

			func_index = returning_func_index.u;
			cur_func = &context->function[func_index];
			cur_frame_size = returning_func_frame_size.u;
			cur_pc = returning_cur_pc.u;

			break;
		}
		case dsh_opcode_mov:
		{
			if (instruction.a >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c] = stack.reg_current[instruction.a];

			break;
		}
		case dsh_opcode_stor:
		{
			if (instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			cur_pc++;

			if (cur_pc == cur_func->bytecode_end)
			{
				fprintf(stderr, "reached the end of function without ret instruction.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].u = *(uint32_t *)(context->bytecode + cur_pc);

			break;
		}

		case dsh_opcode_cmpi_l:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].i < stack.reg_current[instruction.b].i;

			break;
		}
		case dsh_opcode_cmpi_le:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].i <= stack.reg_current[instruction.b].i;

			break;
		}

		case dsh_opcode_cmpf_l:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].f < stack.reg_current[instruction.b].f;

			break;
		}

		case dsh_opcode_cmpf_le:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].f <= stack.reg_current[instruction.b].f;

			break;
		}

		case dsh_opcode_jmp_u:
		{
			uint16_t offset = (instruction.b << 8) | (instruction.c);

			cur_pc += *(int16_t *)&offset;

			if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
			{
				fprintf(stderr, "jmp to outside of the current function.\n");
				goto execution_error;
			}

			// Skip the normal increment

			continue;
		}
		case dsh_opcode_jmp_c:
		{
			if (instruction.a >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[instruction.a].i)
			{
				uint16_t offset = (instruction.b << 8) | (instruction.c);

				cur_pc += *(int16_t *)&offset;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				// Skip the normal increment

				continue;
			}

			break;
		}

		case dsh_opcode_addi:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i =
				stack.reg_current[instruction.a].i +
				stack.reg_current[instruction.b].i;

			break;
		}
		
		case dsh_opcode_addf:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].f =
				stack.reg_current[instruction.a].f +
				stack.reg_current[instruction.b].f;

			break;
		}

		case dsh_opcode_subi:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i =
				stack.reg_current[instruction.a].i -
				stack.reg_current[instruction.b].i;

			break;
		}
		
		case dsh_opcode_subf:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].f =
				stack.reg_current[instruction.a].f -
				stack.reg_current[instruction.b].f;

			break;
		}

		case dsh_opcode_muli:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i =
				stack.reg_current[instruction.a].i *
				stack.reg_current[instruction.b].i;

			break;
		}
		
		case dsh_opcode_mulf:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].f =
				stack.reg_current[instruction.a].f *
				stack.reg_current[instruction.b].f;

			break;
		}

		case dsh_opcode_divi:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i =
				stack.reg_current[instruction.a].i /
				stack.reg_current[instruction.b].i;

			break;
		}
		
		case dsh_opcode_divf:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].f =
				stack.reg_current[instruction.a].f /
				stack.reg_current[instruction.b].f;

			break;
		}

		case dsh_opcode_casti:
		{
			if (instruction.a >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = (int32_t)stack.reg_current[instruction.a].f;

			break;
		}

		case dsh_opcode_castf:
		{
			if (instruction.a >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].f = (float)stack.reg_current[instruction.a].i;

			break;
		}

		default:
			fprintf(stderr, "invalid opcode executed.\n");
			goto execution_error;
		}

		++cur_pc;

		if (cur_pc == cur_func->bytecode_end)
		{
			fprintf(stderr, "reached the end of function without ret instruction.\n");
			goto execution_error;
		}
	}

execution_over:

	dsh_stack_dealloc(&stack);

	return 1;

execution_error:

	dsh_stack_dealloc(&stack);

	return 0;
}