#ifndef dsh_bytecode_h
#define dsh_bytecode_h

#include <stdint.h>

enum dsh_opcode
{
	dsh_opcode_nop = 0,

	dsh_opcode_call = 1,
	dsh_opcode_ret = 2,
	dsh_opcode_mov = 3,
	dsh_opcode_stor = 4,

	dsh_opcode_cmpi_l = 5,
	dsh_opcode_cmpf_l = 6,

	dsh_opcode_cmpi_le = 7,
	dsh_opcode_cmpf_le = 8,

	dsh_opcode_jmp_c = 9,
	dsh_opcode_jmp_cn = 10,
	dsh_opcode_jmp_u = 11,
		
	dsh_opcode_addi = 12,
	dsh_opcode_addf = 13,
	
	dsh_opcode_subi = 14,
	dsh_opcode_subf = 15,

	dsh_opcode_muli = 16,
	dsh_opcode_mulf = 17,

	dsh_opcode_divi = 18,
	dsh_opcode_divf = 19,

	dsh_opcode_casti = 20,
	dsh_opcode_castf = 21,
};

struct dsh_bc
{
	unsigned int opcode : 8;
	unsigned int a : 8;
	unsigned int b : 8;
	unsigned int c : 8;
};
typedef struct dsh_bc dsh_bc;

#endif