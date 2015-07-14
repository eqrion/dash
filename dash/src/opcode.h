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

	dsh_opcode_jmpi_e = 5,
	dsh_opcode_jmpi_l = 6,
	dsh_opcode_jmpi_le = 7,
	dsh_opcode_jmpi_g = 8,
	dsh_opcode_jmpi_ge = 9,
	
	dsh_opcode_jmpf_e = 10,
	dsh_opcode_jmpf_l = 11,
	dsh_opcode_jmpf_le = 12,
	dsh_opcode_jmpf_g = 13,
	dsh_opcode_jmpf_ge = 14,

	dsh_opcode_addi = 15,
	dsh_opcode_addf = 16,

	dsh_opcode_subi = 17,
	dsh_opcode_subf = 18,

	dsh_opcode_muli = 19,
	dsh_opcode_mulf = 20,

	dsh_opcode_divi = 21,
	dsh_opcode_divf = 22,
};

uint32_t dsh_emit_instruction(uint8_t opcode, uint8_t source1, uint8_t source2, uint8_t destination)
{
	return (((uint32_t)destination) << 24) | (((uint32_t)source2) << 16) | (((uint32_t)source1) << 8) | (((uint32_t)opcode));
}

uint8_t dsh_decode_destination(uint32_t instruction)
{
	return (uint8_t)(instruction >> 24);
}
uint8_t dsh_decode_source2(uint32_t instruction)
{
	return (uint8_t)((instruction >> 16) & 255);
}
uint8_t dsh_decode_source1(uint32_t instruction)
{
	return (uint8_t)((instruction >> 8) & 255);
}
uint8_t dsh_decode_opcode(uint32_t instruction)
{
	return (uint8_t)((instruction) & 255);
}

uint32_t dsh_emit_nop()
{
	return dsh_emit_instruction(dsh_opcode_nop, 0, 0, 0);
}

#endif