#include "dash/vm.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	struct dsh_lib *obj = NULL;

	if (!dsh_load_lib(&obj, "out.dob"))
	{
		fprintf(stderr, "error loading out.dob.\n");
		return 1;
	}

	uint32_t in = 1;
	uint32_t out;

	if (dsh_exec_func(obj, 0, &in, &out))
	{
		printf("function succeeded and the result is: %u", out);
	}

	if (obj != NULL)
	{
		dsh_destroy_lib(obj);
		obj = NULL;
	}

	return 0;
}