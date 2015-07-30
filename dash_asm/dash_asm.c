#include "dash/context.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	/*if (argc != 3)
	{
		printf("dash assembler\nusage:\n\tdash_asm <src_filename> <out_filename>\n");
		return 1;
	}

	dsh_assemble(argv[1], argv[2]);*/
	
	if (argc != 2)
	{
		printf("dash compiler\nusage:\n\tdash_compiler <src_filename>\n");
		return 1;
	}

	struct dsh_context *context = NULL;

	if (!dsh_create_context(&context, 1, 64))
	{
		fprintf(stderr, "error initializing dash.");
		return 1;
	}

	if (dsh_context_import_source(argv[1], context))
	{
		dsh_var in[2];
		dsh_var out[1];

		in[0].f = 5;
		in[1].f = 7;
		
		struct dsh_function_def *func = dsh_context_find_func("mult_plus_one", context);

		dsh_context_dissasm_func(func, stdout, context);

		if (dsh_context_exec_func(func, in, out, context))
		{
			printf("f(%f, %f) = %f\n", in[0].f, in[1].f, out[0].f);
		}
		else
		{
			fprintf(stderr, "execution error\n");
		}
	}
	else
	{
		fprintf(stderr, "compilation error\n");
	}

	return 0;
}