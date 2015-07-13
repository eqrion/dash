#include "opcode.h"
#include <stdlib.h>
#include <stdio.h>

struct dsh_function_def
{
	uint16_t reg_count_in;
	uint16_t reg_count_use;
	uint16_t reg_count_out;

	uint32_t bytecode_start;
	uint32_t bytecode_end;
};

struct dsh_obj
{
	uint32_t				  function_count;
	struct dsh_function_def  *function;

	uint32_t	bytecode_count;
	uint32_t*	bytecode;
};

int dsh_verify_obj(struct dsh_obj *obj)
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

int dsh_load_obj(struct dsh_obj **obj, const char *dob_file)
{
	// Open the object binary file

	FILE *obj_file = fopen(dob_file, "rb");
	
	if (obj_file == NULL)
	{
		fprintf(stderr, "cannot open obj file.\n");
		return 0;
	}

	// Allocate the dsh_object

	struct dsh_obj *result = (struct dsh_obj*)malloc(sizeof(struct dsh_obj));

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
	fseek(obj_file, -(int)(result->function_count) * (sizeof(uint32_t) * 2 + sizeof(uint16_t) * 3) - (int)sizeof(uint32_t) * 2, SEEK_CUR);
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

		fread(&function->reg_count_in, sizeof(uint16_t), 1, obj_file);
		fread(&function->reg_count_use, sizeof(uint16_t), 1, obj_file);
		fread(&function->reg_count_out, sizeof(uint16_t), 1, obj_file);
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

	if (!dsh_verify_obj(result))
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

int dsh_exec_obj(struct dsh_obj *obj, uint32_t function_index, const uint32_t *in_registers, uint32_t *out_registers)
{

}

void dsh_destroy_obj(struct dsh_obj *obj)
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