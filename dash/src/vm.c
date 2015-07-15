#include "dash/vm.h"

#include "bytecode.h"
#include "stack.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct dsh_function_def
{
	uint8_t reg_count_in;
	uint8_t reg_count_use;
	uint8_t reg_count_out;

	uint32_t bytecode_start;
	uint32_t bytecode_end;
};

struct dsh_lib
{
	uint32_t				  function_count;
	struct dsh_function_def  *function;

	uint32_t	bytecode_count;
	uint32_t*	bytecode;
};

int dsh_verify_lib(struct dsh_lib *obj)
{
	for (uint32_t i = 0; i < obj->function_count; ++i)
	{
		struct dsh_function_def *function = &obj->function[i];

		if (function->bytecode_end <= function->bytecode_start)
		{
			fprintf(stderr, "empty or negative sized function.n");
			return 0;
		}

		if (function->bytecode_start >= obj->bytecode_count ||
			function->bytecode_end > obj->bytecode_count)
		{
			fprintf(stderr, "invalid function size.\n");
			return 0;
		}
	}

	return 1;
}

int dsh_load_lib(struct dsh_lib **obj, const char *dob_file)
{
	// Open the object binary file

	FILE *obj_file = fopen(dob_file, "rb");
	
	if (obj_file == NULL)
	{
		fprintf(stderr, "cannot open obj file.\n");
		return 0;
	}

	// Allocate the dsh_libect

	struct dsh_lib *result = (struct dsh_lib*)malloc(sizeof(struct dsh_lib));

	if (result == NULL)
	{
		fclose(obj_file);

		fprintf(stderr, "error reading obj file.\n");
		return 0;
	}
	
	// Read the dob file's header

	uint32_t dob_version;
	uint32_t dob_function_count;

	fseek(obj_file, -(int)sizeof(uint32_t) * 2, SEEK_END);
	fread(&dob_version, sizeof(uint32_t), 1, obj_file);
	fread(&dob_function_count, sizeof(uint32_t), 1, obj_file);

	if (dob_version != 1)
	{
		fclose(obj_file);
		free(result);

		fprintf(stderr, "incompatible version of dob file.\n");
		return 0;
	}

	// Extract the amount of functions and instructions present in this dob_file

	result->function_count = dob_function_count;
	fseek(obj_file, -(int)(result->function_count) * (sizeof(uint32_t) * 2 + sizeof(uint8_t) * 3) - (int)sizeof(uint32_t) * 2, SEEK_CUR);
	result->bytecode_count = ftell(obj_file) / 4;

	// Read in the function definitions

	result->function = (struct dsh_function_def *)malloc(sizeof(struct dsh_function_def) * result->function_count);
	
	if (result->function == NULL)
	{
		free(result);
		fclose(obj_file);

		fprintf(stderr, "error allocating space for obj file.\n");
		return 0;
	}
	
	for (uint32_t i = 0; i < result->function_count; ++i)
	{
		struct dsh_function_def *function = &result->function[i];

		fread(&function->reg_count_in, sizeof(uint8_t), 1, obj_file);
		fread(&function->reg_count_use, sizeof(uint8_t), 1, obj_file);
		fread(&function->reg_count_out, sizeof(uint8_t), 1, obj_file);
		fread(&function->bytecode_start, sizeof(uint32_t), 1, obj_file);
		fread(&function->bytecode_end, sizeof(uint32_t), 1, obj_file);
	}

	// Read in the bytecode

	result->bytecode = (uint32_t *)malloc(sizeof(uint32_t) * result->bytecode_count);
	
	if (result->bytecode == NULL)
	{
		free(result->function);
		free(result);
		fclose(obj_file);

		fprintf(stderr, "error allocating space for obj file.\n");
		return 0;
	}
	
	fseek(obj_file, 0, SEEK_SET);
	fread(result->bytecode, sizeof(uint32_t), result->bytecode_count, obj_file);

	// Verify the integrity of this obj file

	if (!dsh_verify_lib(result))
	{
		free(result->bytecode);
		free(result->function);
		free(result);
		fclose(obj_file);

		return 0;
	}

	fclose(obj_file);
	*obj = result;

	return 1;
}

int dsh_exec_func(struct dsh_lib *lib, uint32_t func_index, const dsh_var *func_parameters, dsh_var *func_results)
{
	if (func_index >= lib->function_count)
	{
		fprintf(stderr, "invalid function index to execute.\n");
		return 0;
	}

	// Execution variables 

	uint32_t					cur_func_index = func_index;
	struct dsh_function_def    *cur_func = &lib->function[func_index];
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
		uint32_t instruction = lib->bytecode[cur_pc];
		uint8_t opcode, source1, source2, destination;

		opcode = dsh_decode_opcode(instruction);
		source1 = dsh_decode_source1(instruction);
		source2 = dsh_decode_source2(instruction);
		destination = dsh_decode_destination(instruction);

		switch (opcode)
		{
		case dsh_opcode_nop:
			break;

		case dsh_opcode_call:
		{
			// Validate the operands:
			//		source1 -> call_func_index
			//		source2 -> call_in_register_start
			//		destination -> call_out_register_start

			if (source1 >= lib->function_count)
			{
				fprintf(stderr, "invalid function referenced in call.\n");
				goto execution_error;
			}

			struct dsh_function_def *next_func = &lib->function[source1];

			if (source2 >= cur_frame_size ||
				source2 + next_func->reg_count_in > cur_frame_size)
			{
				fprintf(stderr, "invalid register range for function parameters.\n");
				goto execution_error;
			}

			if (destination >= cur_frame_size ||
				destination + next_func->reg_count_out > cur_frame_size)
			{
				fprintf(stderr, "invalid register range for return data.\n");
				goto execution_error;
			}

			// Grab the pointer for where to get the parameters, before we push an activation record

			dsh_var *reg_in_start = &stack.reg_current[source2];

			// Push space for an activation record

			if (!dsh_stack_push(&stack, 4))
			{
				fprintf(stderr, "stack overflow error.\n");
				goto execution_error;
			}

			stack.reg_current[0].u = cur_frame_size;
			stack.reg_current[1].u = cur_func_index;
			stack.reg_current[2].u = destination;
			stack.reg_current[3].u = cur_pc;

			// Switch to the new function

			cur_func_index = source1;
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
			// Validate the operands (source1 = reg_out_start)

			if (source1 >= cur_frame_size ||
				source1 + cur_func->reg_count_out > cur_frame_size)
			{
				fprintf(stderr, "invalid register range for result registers.\n");
				goto execution_error;
			}

			// Grab a pointer to the source for return data

			dsh_var *reg_out_start = &stack.reg_current[source1];

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
			cur_func = &lib->function[func_index];
			cur_frame_size = returning_func_frame_size.u;
			cur_pc = returning_cur_pc.u;

			break;
		}
		case dsh_opcode_mov:
		{
			if (source1 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination] = stack.reg_current[source1];

			break;
		}
		case dsh_opcode_movc:
		{
			if (source1 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].u = source1;

			break;
		}

		case dsh_opcode_jmp:
		{
			cur_pc += *(int8_t *)&destination;

			if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
			{
				fprintf(stderr, "jmp to outside of the current function.\n");
				goto execution_error;
			}

			// Skip the normal increment

			continue;
		}

		case dsh_opcode_jmp_ie:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}
			
			if (stack.reg_current[source1].i == stack.reg_current[source2].i)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_il:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i < stack.reg_current[source2].i)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_ile:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i <= stack.reg_current[source2].i)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_ig:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i > stack.reg_current[source2].i)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_ige:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i >= stack.reg_current[source2].i)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}

		case dsh_opcode_jmp_ice:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i == *((int8_t *)&source2))
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_icl:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i < *((int8_t *)&source2))
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_icle:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i <= *((int8_t *)&source2))
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_icg:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i > *((int8_t *)&source2))
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_icge:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].i >= *((int8_t *)&source2))
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		
		case dsh_opcode_jmp_fe:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].f == stack.reg_current[source2].f)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_fl:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].f < stack.reg_current[source2].f)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_fle:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].f <= stack.reg_current[source2].f)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_fg:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].f > stack.reg_current[source2].f)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}
		case dsh_opcode_jmp_fge:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			if (stack.reg_current[source1].f >= stack.reg_current[source2].f)
			{
				cur_pc += *(int8_t *)&destination;

				if (cur_pc >= cur_func->bytecode_end || cur_pc < cur_func->bytecode_start)
				{
					fprintf(stderr, "jmp to outside of the current function.\n");
					goto execution_error;
				}

				continue;
			}

			break;
		}

		case dsh_opcode_addi:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i +
				stack.reg_current[source2].i;

			break;
		}
		case dsh_opcode_addic:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i +
				*((int8_t *)&source2);

			break;
		}
		case dsh_opcode_addf:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].f =
				stack.reg_current[source1].f +
				stack.reg_current[source2].f;

			break;
		}

		case dsh_opcode_subi:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i -
				stack.reg_current[source2].i;

			break;
		}
		case dsh_opcode_subic:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i -
				*((int8_t *)&source2);

			break;
		}
		case dsh_opcode_subf:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].f =
				stack.reg_current[source1].f -
				stack.reg_current[source2].f;

			break;
		}

		case dsh_opcode_muli:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i *
				stack.reg_current[source2].i;

			break;
		}
		case dsh_opcode_mulic:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i *
				*((int8_t *)&source2);

			break;
		}
		case dsh_opcode_mulf:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].f =
				stack.reg_current[source1].f *
				stack.reg_current[source2].f;

			break;
		}

		case dsh_opcode_divi:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i /
				stack.reg_current[source2].i;

			break;
		}
		case dsh_opcode_divic:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].i =
				stack.reg_current[source1].i /
				*((int8_t *)&source2);

			break;
		}
		case dsh_opcode_divf:
		{
			if (source1 >= cur_frame_size || source2 >= cur_frame_size || destination >= cur_frame_size)
			{
				fprintf(stderr, "register out of bounds error.\n");
				goto execution_error;
			}

			stack.reg_current[destination].f =
				stack.reg_current[source1].f /
				stack.reg_current[source2].f;

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

void dsh_destroy_lib(struct dsh_lib *obj)
{
	if (obj->bytecode != NULL)
	{
		free(obj->bytecode);
		obj->bytecode = NULL;
	}
	if (obj->function != NULL)
	{
		free(obj->function);
		obj->function = NULL;
	}
	free(obj);
}