#include "dash/vm.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	if (argc != 2 && argc != 3)
	{
		printf("dash\nusage:\n\tdash [-d] <src_filename>\n");
		return 1;
	}

	if (argc == 3 && strcmp(argv[1], "-d") != 0)
	{
		printf("dash\nusage:\n\tdash [-d] <src_filename>\n");
		return 1;
	}

	struct dvm_context *context = NULL;

	if (!dvm_create_context(&context, 4, 128))
	{
		fprintf(stderr, "error initializing dash.\n");
		return 1;
	}

	if (!dvm_import_source(argc == 2 ? argv[1] : argv[2], context))
	{
		dvm_destroy_context(context);

		fprintf(stderr, "compilation error.\n");
		return 1;
	}

	struct dvm_procedure *func = dvm_find_proc("main", 0, 1, context);
	
	if (func == NULL)
	{
		dvm_destroy_context(context);

		fprintf(stderr, "couldn't find a main function.\n");
		return 1;
	}
	
	if (argc == 3)
	{
		fprintf(stdout, "\n");
		dvm_dissasm_proc(func, stdout, context);
	}
	else
	{
		dvm_var out[1];

		if (dvm_exec_proc(func, NULL, out, context))
		{
			fprintf(stdout, "main returned with %d.\n", out[0].i);
		}
		else
		{
			fprintf(stderr, "execution error.\n");
		}
	}

	dvm_destroy_context(context);
	
	_CrtCheckMemory();
	_CrtDumpMemoryLeaks();

	return 0;
}