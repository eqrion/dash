#include "dash/vm.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("dash compiler\nusage:\n\tdash_compiler <src_filename>\n");
		return 1;
	}

	struct dvm_context *context = NULL;

	if (!dvm_create_context(&context, 1, 64))
	{
		fprintf(stderr, "error initializing dash.");
		return 1;
	}

	if (dvm_import_source(argv[1], context))
	{
		dvm_var in[1];
		dvm_var out[1];

		in[0].i = 10000000;
		
		struct dvm_procedure *func = dvm_find_proc("pi", context);

		dvm_dissasm_proc(func, stdout, context);

		printf("\n");

		if (dvm_exec_proc(func, in, out, context))
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