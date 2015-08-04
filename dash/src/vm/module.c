#include "../vm_internal.h"

#include <stdio.h>

int dvm_import_module(const char *module_filename, dvm_context *context)
{
	// Open the object binary file

	FILE *module_file = fopen(module_filename, "rb");

	if (module_file == NULL)
	{
		fprintf(stderr, "cannot open context file.\n");
		return 0;
	}

	// Read the module file's header

	uint32_t module_file_version;
	uint32_t module_extern_count;
	uint32_t module_function_count;
	uint32_t module_bytecode_count;

	fread(&module_file_version, sizeof(uint32_t), 1, module_file);
	fread(&module_extern_count, sizeof(uint32_t), 1, module_file);
	fread(&module_function_count, sizeof(uint32_t), 1, module_file);
	fread(&module_bytecode_count, sizeof(uint32_t), 1, module_file);

	if (module_file_version != 1)
	{
		fclose(module_file);

		fprintf(stderr, "incompatible version of module file.\n");
		return 0;
	}

	if (module_extern_count > 16)
	{
		fclose(module_file);

		return 0;
	}

	// Read in the extern function table

	struct dsh_extern
	{
		uint32_t hashed_name;
		uint8_t reg_count_in;
		uint8_t reg_count_out;

		size_t resolved_function_index;
	};

	struct dsh_extern *extern_table = _alloca(
		sizeof(struct dsh_extern) * module_extern_count
		);

	for (uint32_t i = 0; i < module_extern_count; ++i)
	{
		fread(&extern_table[i].hashed_name, sizeof(uint32_t), 1, module_file);
		fread(&extern_table[i].reg_count_in, sizeof(uint8_t), 1, module_file);
		fread(&extern_table[i].reg_count_out, sizeof(uint8_t), 1, module_file);

		extern_table[i].resolved_function_index = dvm_find_proc_index(extern_table[i].hashed_name, context);

		if (extern_table[i].resolved_function_index == ~0)
		{
			fclose(module_file);

			return 0;
		}
	}

	// Read in the function table

	size_t func_base = context->function_count;

	if (!dvm_context_push_function(module_function_count, context))
	{
		fclose(module_file);

		return 0;
	}

	for (uint32_t i = 0; i < module_function_count; ++i)
	{
		dvm_procedure *cur = &context->function[func_base + i];

		fread(&cur->hashed_name, sizeof(uint32_t), 1, module_file);
		fread(&cur->bytecode_start, sizeof(uint32_t), 1, module_file);
		fread(&cur->bytecode_end, sizeof(uint32_t), 1, module_file);
		fread(&cur->reg_count_in, sizeof(uint8_t), 1, module_file);
		fread(&cur->reg_count_use, sizeof(uint8_t), 1, module_file);
		fread(&cur->reg_count_out, sizeof(uint8_t), 1, module_file);

		if (cur->bytecode_end <= cur->bytecode_start)
		{
			fprintf(stderr, "empty or negative sized function.n");
			return 0;
		}

		if (cur->bytecode_start >= module_bytecode_count ||
			cur->bytecode_end > module_bytecode_count)
		{
			fprintf(stderr, "invalid function size.\n");
			return 0;
		}

		cur->bytecode_start += context->bytecode_count;
		cur->bytecode_end += context->bytecode_count;
	}

	// Read in the bytecode

	size_t bc_base = context->bytecode_count;

	if (!dvm_context_push_bytecode(module_bytecode_count, context))
	{
		dvm_context_pop_function(module_function_count, context);
		fclose(module_file);

		return 0;
	}

	fread(context->bytecode + bc_base, sizeof(dvm_bc), module_bytecode_count, module_file);

	// Verify and link the bytecode

	for (size_t i = bc_base; i < context->bytecode_count; ++i)
	{
		dvm_bc *current = &context->bytecode[i];

		if (current->opcode == dvm_opcode_stor)
		{
			++i;
		}
		else if (current->opcode == dvm_opcode_call)
		{
			if (current->a < module_extern_count)
			{
				current->a = extern_table[current->a].resolved_function_index;
			}
			else
			{
				current->a += func_base;
			}
		}
	}

	fclose(module_file);

	return 1;
}