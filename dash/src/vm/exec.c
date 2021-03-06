#include "../vm_internal.h"
#include "../hash.h"

#include "stack.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int dvm_exec_proc(struct dvm_procedure *function, const dvm_var *func_parameters, dvm_var *func_results, struct dvm_context *context)
{
	if (function == NULL)
	{
		fprintf(stderr, "invalid function.\n");
		return 0;
	}

	// Execution variables 

	uint32_t					 cur_func_index = function - context->function;
	struct dvm_procedure		*cur_func = &context->function[cur_func_index];
	uint32_t					 cur_pc = cur_func->bytecode_start;
	uint32_t					 cur_frame_size = cur_func->reg_count_in + cur_func->reg_count_use;

	// Allocate the stack for use in executing this function

	struct dvm_stack stack;
	memset(&stack, 0, sizeof(stack));

	if (!dvm_stack_alloc(&stack, 128))
	{
		fprintf(stderr, "error creating a dash stack.\n");
		return 0;
	}
	
	// Push the first function's registers, and parameters over

	if (!dvm_stack_push(&stack, cur_frame_size))
	{
		fprintf(stderr, "stack overflow error.\n");
		return 0;
	}

	memcpy(
		stack.reg_current,
		func_parameters,
		sizeof(dvm_var) * cur_func->reg_count_in
		);

	// Bytecode execution main loop

	while (1)
	{
		dvm_bc instruction = context->bytecode[cur_pc];
		
		switch (instruction.opcode)
		{
		case dvm_opcode_nop:
			break;

		case dvm_opcode_call:
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

			struct dvm_procedure *next_func = &context->function[instruction.a];

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

			dvm_var *reg_in_start = &stack.reg_current[instruction.b];

			if (next_func->c_function != NULL)
			{
				dvm_var *reg_out_start = &stack.reg_current[instruction.c];

				next_func->c_function(reg_in_start, reg_out_start);
			}
			else
			{
				// Push space for an activation record

				if (!dvm_stack_push(&stack, 4))
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

				if (!dvm_stack_push(&stack, cur_frame_size))
				{
					fprintf(stderr, "stack overflow error.\n");
					goto execution_error;
				}

				// Copy the parameters over

				memcpy(
					stack.reg_current,
					reg_in_start,
					sizeof(dvm_var) * cur_func->reg_count_in
					);

				// Skip over the pc increment and continue on

				continue;
			}

			break;
		}
		case dvm_opcode_ret:
		{
			// Validate the operands (instruction.a = reg_out_start)

			if (instruction.a >= cur_frame_size ||
				(uint32_t)(instruction.a + cur_func->reg_count_out) > (uint32_t)(cur_frame_size))
			{
				fprintf(stderr, "invalid register range for result registers.\n");
				goto execution_error;
			}

			// Grab an pointer to the source for return data

			dvm_var *reg_out_start = &stack.reg_current[instruction.a];

			// Pop this function's registers off the stack

			if (!dvm_stack_pop(&stack, cur_frame_size))
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
					sizeof(dvm_var) * cur_func->reg_count_out
					);

				goto execution_over;
			}

			// We need to return back to our previous function,
			// grab the activation record from the stack and pop past it

			dvm_var returning_func_frame_size = stack.reg_current[0];
			dvm_var returning_func_index = stack.reg_current[1];
			dvm_var returning_func_result_dest = stack.reg_current[2];
			dvm_var returning_cur_pc = stack.reg_current[3];

			if (!dvm_stack_pop(&stack, 4))
			{
				fprintf(stderr, "stack underflow error.\n");
				goto execution_error;
			}

			// The stack is now back to where it was, now we just
			// need to copy the result, and move the variables back.

			memcpy(
				stack.reg_current + returning_func_result_dest.u,
				reg_out_start,
				sizeof(dvm_var) * cur_func->reg_count_out
				);

			cur_func_index = returning_func_index.u;
			cur_func = &context->function[cur_func_index];
			cur_frame_size = returning_func_frame_size.u;
			cur_pc = returning_cur_pc.u;

			break;
		}
		case dvm_opcode_mov:
		{
			if (instruction.a >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c] = stack.reg_current[instruction.a];

			break;
		}
		case dvm_opcode_stor:
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

		case dvm_opcode_and:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].i & stack.reg_current[instruction.b].i;

			break;
		}
		case dvm_opcode_or:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].i | stack.reg_current[instruction.b].i;

			break;
		}
		case dvm_opcode_not:
		{
			if (instruction.a >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = !stack.reg_current[instruction.a].i;

			break;
		}

		case dvm_opcode_cmpi_e:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].i == stack.reg_current[instruction.b].i;

			break;
		}
		case dvm_opcode_cmpf_e:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].f == stack.reg_current[instruction.b].f;

			break;
		}

		case dvm_opcode_cmpi_l:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].i < stack.reg_current[instruction.b].i;

			break;
		}
		case dvm_opcode_cmpi_le:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].i <= stack.reg_current[instruction.b].i;

			break;
		}

		case dvm_opcode_cmpf_l:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].f < stack.reg_current[instruction.b].f;

			break;
		}

		case dvm_opcode_cmpf_le:
		{
			if (instruction.a >= cur_frame_size || instruction.b >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = stack.reg_current[instruction.a].f <= stack.reg_current[instruction.b].f;

			break;
		}

		case dvm_opcode_jmp_c:
		{
			if (instruction.a >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[instruction.a].i)
			{
				uint8_t offset = instruction.c;

				cur_pc += *(int8_t *)&offset;

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
		case dvm_opcode_jmp_cn:
		{
			if (instruction.a >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (!stack.reg_current[instruction.a].i)
			{
				uint8_t offset = instruction.c;

				cur_pc += *(int8_t *)&offset;

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
		case dvm_opcode_jmp_u:
		{
			uint8_t offset = instruction.c;

			cur_pc += *(int8_t *)&offset;

			if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
			{
				fprintf(stderr, "jmp to outside of the current function.\n");
				goto execution_error;
			}

			// Skip the normal increment

			continue;
		}

		case dvm_opcode_addi:
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
		
		case dvm_opcode_addf:
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

		case dvm_opcode_subi:
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
		
		case dvm_opcode_subf:
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

		case dvm_opcode_muli:
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
		
		case dvm_opcode_mulf:
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

		case dvm_opcode_divi:
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
		
		case dvm_opcode_divf:
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

		case dvm_opcode_casti:
		{
			if (instruction.a >= cur_frame_size || instruction.c >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[instruction.c].i = (int32_t)stack.reg_current[instruction.a].f;

			break;
		}

		case dvm_opcode_castf:
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

	dvm_stack_dealloc(&stack);

	return 1;

execution_error:

	dvm_stack_dealloc(&stack);

	return 0;
}