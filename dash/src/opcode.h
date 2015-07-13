#ifndef dsh_opcode_h
#define dsh_opcode_h

#include <stdint.h>

enum dsh_opcode
{
	dsh_opcode_nop = 0,

	dsh_opcode_call = 1,
	dsh_opcode_ret = 2,
	dsh_opcode_mov = 3,

	dsh_opcode_jmp = 4,
	dsh_opcode_jmpe = 5,
	dsh_opcode_jmpl = 6,
	dsh_opcode_jmpg = 7,

	dsh_opcode_cmpi = 8,
	dsh_opcode_cmpu = 9,
	dsh_opcode_cmpf = 10,
	
	dsh_opcode_addi = 11,
	dsh_opcode_addu = 12,
	dsh_opcode_addf = 13,

	dsh_opcode_subi = 14,
	dsh_opcode_subu = 15,
	dsh_opcode_subf = 16,

	dsh_opcode_muli = 17,
	dsh_opcode_mulu = 18,
	dsh_opcode_mulf = 19,

	dsh_opcode_divi = 20,
	dsh_opcode_divu = 21,
	dsh_opcode_divf = 22,
};

uint32_t dsh_emit_instruction(uint8_t opcode, uint8_t source1, uint8_t source2, uint8_t destination)
{
	return (((uint32_t)destination) << 24) | (((uint32_t)source1) << 16) | (((uint32_t)opcode) << 8) | (((uint32_t)destination));
}

uint32_t dsh_emit_nop()
{
	return dsh_emit_instruction(dsh_opcode_nop, 0, 0, 0);
}

#endif