#include "dash/compiler.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	/*if (argc != 3)
	{
		printf("dash assembler\nusage:\n\tdash_asm <src_filename> <out_filename>\n");
		return 1;
	}

	dsh_assemble(argv[1], argv[2]);*/
	
	if (argc != 3)
	{
		printf("dash compiler\nusage:\n\tdash_compiler <src_filename> <out_filename>\n");
		return 1;
	}

	dsh_compile_source(argv[1], argv[2]);

	return;
}