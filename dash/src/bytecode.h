#ifndef dsh_bytecode_h
#define dsh_bytecode_h

#include <stdint.h>

enum dsh_opcode
{
	dsh_opcode_nop = 0,

	dsh_opcode_call = 1,
	dsh_opcode_ret = 2,
	dsh_opcode_mov = 3,
	dsh_opcode_movc = 4,

	dsh_opcode_jmp = 5,

	dsh_opcode_jmp_ie = 6,
	dsh_opcode_jmp_il = 7,
	dsh_opcode_jmp_ile = 8,
	dsh_opcode_jmp_ig = 9,
	dsh_opcode_jmp_ige = 10,

	dsh_opcode_jmp_ice = 11,
	dsh_opcode_jmp_icl = 12,
	dsh_opcode_jmp_icle = 13,
	dsh_opcode_jmp_icg = 14,
	dsh_opcode_jmp_icge = 15,

	dsh_opcode_jmp_fe = 16,
	dsh_opcode_jmp_fl = 17,
	dsh_opcode_jmp_fle = 18,
	dsh_opcode_jmp_fg = 19,
	dsh_opcode_jmp_fge = 20,

	dsh_opcode_addi = 21,
	dsh_opcode_addic = 22,
	dsh_opcode_addf = 23,
	
	dsh_opcode_subi = 24,
	dsh_opcode_subic = 25,
	dsh_opcode_subf = 26,

	dsh_opcode_muli = 27,
	dsh_opcode_mulic = 28,
	dsh_opcode_mulf = 29,

	dsh_opcode_divi = 30,
	dsh_opcode_divic = 31,
	dsh_opcode_divf = 32,
};

inline uint32_t dsh_emit_instruction(uint8_t opcode, uint8_t source1, uint8_t source2, uint8_t destination)
{
	return 
		(((uint32_t)destination) << 24) |
		(((uint32_t)source2) << 16) |
		(((uint32_t)source1) << 8) |
		(((uint32_t)opcode));
}

inline uint8_t dsh_decode_destination(uint32_t instruction)
{
	return (uint8_t)(instruction >> 24);
}
inline uint8_t dsh_decode_source2(uint32_t instruction)
{
	return (uint8_t)((instruction >> 16) & 255);
}
inline uint8_t dsh_decode_source1(uint32_t instruction)
{
	return (uint8_t)((instruction >> 8) & 255);
}
inline uint8_t dsh_decode_opcode(uint32_t instruction)
{
	return (uint8_t)((instruction) & 255);
}

inline uint32_t dsh_emit_nop()
{
	return dsh_emit_instruction(dsh_opcode_nop, 0, 0, 0);
}

#endif