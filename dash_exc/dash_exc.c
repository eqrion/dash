#include "dash/vm.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	if (argc != 1 && argc != 2)
	{
		printf("dash\nusage:\n\tdash [-d]\n");
		return 0;
	}

	if (argc == 2 && strcmp(argv[1], "-d") != 0)
	{
		printf("dash\nusage:\n\tdash [-d]\n");
		return 0;
	}

	struct dvm_context *context = NULL;

	if (!dvm_create_context(&context, 4, 128))
	{
		fprintf(stderr, "error initializing dash.\n");
		return 0;
	}

	if (!dvm_import_source(stdin, context))
	{
		dvm_destroy_context(context);

		fprintf(stderr, "compilation error.\n");
		return 0;
	}
		
	if (argc == 2)
	{
		dvm_dissasm_module(stdout, context);
	}
	else
	{
		struct dvm_procedure *func = dvm_find_proc("main", 0, 1, context);

		if (func == NULL)
		{
			dvm_destroy_context(context);

			fprintf(stderr, "couldn't find a main function.\n");
			return 0;
		}

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
	
	/*_CrtCheckMemory();
	_CrtDumpMemoryLeaks();*/

	return 0;
}