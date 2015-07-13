#include "dash/vm.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	struct dsh_obj *obj = NULL;

	if (!dsh_load_obj(&obj, "out.dob"))
	{
		fprintf(stderr, "error loading out.dob.\n");
		return 1;
	}

	if (obj != NULL)
	{
		dsh_destroy_obj(obj);
		obj = NULL;
	}

	return 0;
}