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
		/*for (int n = 0; n <= 10; ++n)
		{
			dvm_var in[1];
			dvm_var out[1];

			in[0].i = n;

			if (dvm_exec_proc(func, in, out, context))
			{
				printf("fib(%d) = %d\n", in[0].i, out[0].i);
			}
			else
			{
				fprintf(stderr, "execution error\n");
				break;
			}
		}*/

		struct dvm_procedure *func = dvm_find_proc("test", context);
		dvm_dissasm_proc(func, stdout, context);

		printf("\n");

		dvm_var out[1];

		if (dvm_exec_proc(func, NULL, out, context))
		{
			printf("test = %d\n", out[0].i);
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