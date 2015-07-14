#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opcode.h"
#include "string.h"

long file_len(FILE *file)
{
	long old = ftell(file);
	fseek(file, 0, SEEK_END);
	long res = ftell(file);
	fseek(file, old, SEEK_SET);
	return res;
}

struct function_def
{
	char	   *name;

	uint16_t	reg_count_in;
	uint16_t	reg_count_use;
	uint16_t	reg_count_out;
	uint32_t	bytecode_start;
	uint32_t	bytecode_end;

	int			is_defined;

	struct function_def *next;
};

struct function_def *find_function(const char *name, struct function_def *list)
{
	while (list != NULL)
	{
		if (strcmp(list->name, name) == 0)
			break;

		list = list->next;
	}
	return list;
}

uint32_t find_function_index(const char *name, struct function_def *list)
{
	uint32_t index = 0;
	struct function_def *result = NULL;

	while (list != NULL)
	{
		if (strcmp(list->name, name) == 0)
		{
			result = list;
			index = 0;
		}

		list = list->next;
		++index;
	}

	return result == NULL ? ~0 : (index - 1);
}

struct function_def *reverse_function_list(struct function_def *list)
{
	if (list == NULL)
		return NULL;

	struct function_def *new_head = list;

	while (new_head->next != NULL)
		new_head = new_head->next;

	while (list->next != NULL)
	{
		struct function_def *prev = NULL;
		struct function_def *cur = list;

		while (cur->next != NULL)
		{
			prev = cur;
			cur = cur->next;
		}
		
		cur->next = prev;
		prev->next = NULL;
	}

	return new_head;
}

uint32_t parse_instruction(const char *line, struct function_def *function_list)
{
	unsigned int source1, source2, destination;

	if (str_starts_with(line, "nop"))
	{
		return dsh_emit_nop();
	}
	else if (str_starts_with(line, "call"))
	{
		char function_name[64];
		if (sscanf(line, "call %63s i%u o%u", &function_name, &source2, &destination) != 1)
			return ~0;

		source1 = find_function_index(function_name, function_list);

		if (source1 == ~0)
			return ~0;

		return dsh_emit_instruction(dsh_opcode_call, source1, source2, destination);
	}
	else if (str_starts_with(line, "ret"))
	{
		if (sscanf(line, "ret o%u", &source1) != 1)
			return ~0;

		return dsh_emit_instruction(dsh_opcode_ret, source1, 0, 0);
	}
	else if (str_starts_with(line, "mov"))
	{
		if (sscanf(line, "mov r%u r%u", &source1, &destination) != 2)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_mov, source1, 0, destination);
	}
	else if (str_starts_with(line, "jmp"))
	{
		int offset;

		if (sscanf(line, "jmp %d", &offset) != 1)
			return ~0;

		return dsh_emit_instruction(dsh_opcode_jmp, 0, 0, *(uint8_t *)&offset);
	}
	else if (str_starts_with(line, "addi"))
	{
		if (sscanf(line, "addi r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_addi, source1, source2, destination);
	}
	else if (str_starts_with(line, "addf"))
	{
		if (sscanf(line, "addf r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_addf, source1, source2, destination);
	}
	else if (str_starts_with(line, "subi"))
	{
		if (sscanf(line, "subi r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_subi, source1, source2, destination);
	}
	else if (str_starts_with(line, "subf"))
	{
		if (sscanf(line, "subf r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_subf, source1, source2, destination);
	}
	else if (str_starts_with(line, "muli"))
	{
		if (sscanf(line, "muli r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_muli, source1, source2, destination);
	}
	else if (str_starts_with(line, "mulf"))
	{
		if (sscanf(line, "mulf r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_mulf, source1, source2, destination);
	}
	else if (str_starts_with(line, "divi"))
	{
		if (sscanf(line, "divi r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_divi, source1, source2, destination);
	}
	else if (str_starts_with(line, "divf"))
	{
		if (sscanf(line, "divf r%u r%u -> r%u", &source1, &source2, &destination) != 3)
			return ~0;
		return dsh_emit_instruction(dsh_opcode_divf, source1, source2, destination);
	}

	return ~0;
}

int dsh_assemble(const char *assembly_filename, const char *obj_filename)
{
	FILE *asm_file, *ops_file;

	// Open the asm file

	asm_file = fopen(assembly_filename, "rb");

	if (asm_file == NULL)
	{
		fprintf(stderr, "cannot open asm file: %s\n", assembly_filename);
		return 0;
	}

	// Open the ops file

	ops_file = fopen(obj_filename, "wb");

	if (ops_file == NULL)
	{
		fclose(asm_file);

		fprintf(stderr, "cannot open ops file: %s\n", obj_filename);
		return 0;
	}

	// Read the asm file into a buffer

	size_t asm_filelen = file_len(asm_file);

	char *asm_buffer = (char *)malloc(asm_filelen + 1);

	if (asm_buffer == NULL)
	{
		fclose(ops_file);
		fclose(asm_file);

		fprintf(stderr, "cannot allocate for asm file\n");
		return 0;
	}
	asm_buffer[asm_filelen] = 0;

	if (fread(asm_buffer, sizeof(char), asm_filelen, asm_file) != asm_filelen)
	{
		fclose(ops_file);
		fclose(asm_file);

		fprintf(stderr, "error reading asm file\n");
		return 0;
	}

	// Parse the instructions, remembering functions as we go along

	struct function_def *root_def = NULL;

	int line_no = 1;
	
	char *line = strtok(asm_buffer, "\n");
	while (line)
	{
		if (str_is_whitespace(line))
		{
			line = strtok(NULL, "\n");
			++line_no;
			continue;
		}

		const char function_name[64];
		unsigned int reg_count_in;
		unsigned int reg_count_use;
		unsigned int reg_count_out;

		int is_declaration = 0;

		if (sscanf(line, ".dash_function %63s in%ur use%ur out%ur",
			function_name,
			&reg_count_in,
			&reg_count_use,
			&reg_count_out) != 4)
		{
			if (sscanf(line, ".dash_function_decl %63s in%ur use%ur out%ur",
				function_name,
				&reg_count_in,
				&reg_count_use,
				&reg_count_out) != 4)
			{
				goto syntax_error;
			}

			is_declaration = 1;
		}
		
		struct function_def *def = find_function(function_name, root_def);

		if (def != NULL)
		{
			if (def->is_defined)
			{
				// Cannot redefine/declare a function that has bytecode already
				goto syntax_error;
			}

			def->is_defined = is_declaration;

			if (def->reg_count_in != reg_count_in ||
				def->reg_count_use != reg_count_use ||
				def->reg_count_out != reg_count_out)
				goto syntax_error;
		}
		else
		{
			def = (struct function_def *)malloc(sizeof(struct function_def));

			int name_length = strlen(function_name);
			char *buffer = (char *)malloc(name_length + 1);
			memcpy(buffer, function_name, name_length);

			def->name = buffer;
			def->name[name_length] = 0;

			def->reg_count_in = reg_count_in;
			def->reg_count_use = reg_count_use;
			def->reg_count_out = reg_count_out;

			def->next = NULL;
		}
		
		if (!is_declaration)
		{
			def->next = root_def;
			root_def = def;

			def->is_defined = 1;
			def->bytecode_start = ftell(ops_file) / sizeof(uint32_t);

			for (
				line = strtok(NULL, "\n"), ++line_no;
				line != NULL && !str_starts_with(line, ".dash_function");
				line = strtok(NULL, "\n"), ++line_no
				)
			{
				if (str_is_whitespace(line))
					continue;

				uint32_t next_instruction = parse_instruction(line, root_def);

				if (next_instruction == ~0)
					goto syntax_error;

				fwrite(&next_instruction, sizeof(uint32_t), 1, ops_file);
			}

			def->bytecode_end = ftell(ops_file) / sizeof(uint32_t);
		}
		else
		{
			line = strtok(NULL, "\n");
			++line_no;
			continue;
		}
	}

	// Reverse the function list so the first added function is the first node

	root_def = reverse_function_list(root_def);

	// Write out the function table

	uint32_t dob_version = 1;
	uint32_t function_count = 0;
	struct function_def *current = root_def;

	while (current != NULL)
	{
		fwrite(&current->reg_count_in, sizeof(uint8_t), 1, ops_file);
		fwrite(&current->reg_count_use, sizeof(uint8_t), 1, ops_file);
		fwrite(&current->reg_count_out, sizeof(uint8_t), 1, ops_file);
		fwrite(&current->bytecode_start, sizeof(uint32_t), 1, ops_file);
		fwrite(&current->bytecode_end, sizeof(uint32_t), 1, ops_file);

		function_count += 1;
		current = current->next;
	}

	// Write out the obj header

	fwrite(&dob_version, sizeof(uint32_t), 1, ops_file);
	fwrite(&function_count, sizeof(uint32_t), 1, ops_file);

	// Clean up our resource

	while (root_def != NULL)
	{
		struct function_def *next = root_def->next;

		free(root_def->name);
		free(root_def);

		root_def = next;
	}

	free(asm_buffer);
	fclose(ops_file);
	fclose(asm_file);

	return 1;

syntax_error:

	fprintf(stderr, "syntax error on line %d: %s\n", line_no, line);

	free(asm_buffer);
	fclose(ops_file);
	fclose(asm_file);

	while (root_def != NULL)
	{
		struct function_def *next = root_def->next;

		free(root_def->name);
		free(root_def);

		root_def = next;
	}

	return 0;
}