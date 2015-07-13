#include "dash/assembler.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		printf("dash assembler\nusage:\n\tdash_asm <src_filename> <out_filename>\n");
		return 1;
	}

	return dsh_assemble(argv[1], argv[2]);
}