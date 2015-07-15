#include "dash/assembler.h"
#include "dash/vm.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("dash exc\nusage:\n\tdash_exc <src_filename>\n");
		return 1;
	}

	if (!dsh_assemble(argv[1], "out.dib"))
	{
		fprintf(stderr, "error compiling source file. terminating.\n");
		return 2;
	}

	struct dsh_lib *obj = NULL;
	
	if (!dsh_load_lib(&obj, "out.dib"))
	{
		fprintf(stderr, "error loading out.dib.\n");
		return 3;
	}

	dsh_var in[2];
	dsh_var out;

	in[0].i = 2;
	in[1].i = 0;
	out.i = 0;

	while (in[1].i < 20)
	{
		if (dsh_exec_func(obj, 0, in, &out))
		{
			printf("f(%i, %i) = %i\n", in[0].i, in[1].i, out.i);
		}
		else
		{
			fprintf(stderr, "execution error, terminating.\n");
			break;
		}

		++in[1].i;
	}

	if (obj != NULL)
	{
		dsh_destroy_lib(obj);
		obj = NULL;
	}

	return 0;
}