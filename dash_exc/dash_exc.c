#include "dash/context.h"

#include <stdio.h>

int main(int argc, char **argv)
{
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
		dsh_var in[1];
		dsh_var out[1];

		in[0].i = 10000000;
		
		struct dsh_function_def *func = dsh_context_find_func("pi", context);

		dsh_context_dissasm_func(func, stdout, context);

		printf("\n");

		if (dsh_context_exec_func(func, in, out, context))
		{
			printf("pi = %f\n", out[0].f);
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