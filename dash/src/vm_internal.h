#ifndef dsh_vm_internal_h
#define dsh_vm_internal_h

#include <stdint.h>

#include "dash/var.h"
#include "dash/vm.h"

enum dvm_opcode
{
	dvm_opcode_nop = 0,

	dvm_opcode_call = 1,
	dvm_opcode_ret = 2,
	dvm_opcode_mov = 3,
	dvm_opcode_stor = 4,

	dvm_opcode_cmpi_l = 5,
	dvm_opcode_cmpf_l = 6,

	dvm_opcode_cmpi_le = 7,
	dvm_opcode_cmpf_le = 8,

	dvm_opcode_jmp_c = 9,
	dvm_opcode_jmp_cn = 10,
	dvm_opcode_jmp_u = 11,

	dvm_opcode_addi = 12,
	dvm_opcode_addf = 13,

	dvm_opcode_subi = 14,
	dvm_opcode_subf = 15,

	dvm_opcode_muli = 16,
	dvm_opcode_mulf = 17,

	dvm_opcode_divi = 18,
	dvm_opcode_divf = 19,

	dvm_opcode_casti = 20,
	dvm_opcode_castf = 21,
};

struct dvm_bc
{
	unsigned int opcode : 8;
	unsigned int a : 8;
	unsigned int b : 8;
	unsigned int c : 8;
};
typedef struct dvm_bc dvm_bc;

typedef void (*dvm_c_function)(const dvm_var *in_registers, dvm_var *out_registers);
struct dvm_procedure
{
	uint32_t hashed_name;

	uint8_t reg_count_in;
	uint8_t reg_count_use;
	uint8_t reg_count_out;

	dvm_c_function c_function;

	uint32_t bytecode_start;
	uint32_t bytecode_end;
};
typedef struct dvm_procedure dvm_procedure;

struct dvm_context
{
	uint32_t				 function_capacity;
	uint32_t				 function_count;
	dvm_procedure			*function;

	uint32_t	 bytecode_capacity;
	uint32_t	 bytecode_count;
	dvm_bc		*bytecode;
};
typedef struct dvm_context dvm_context;

struct dvm_procedure_emitter
{
	uint32_t bytecode_start;
	uint32_t bytecode_allocated;

	dvm_context *context;
};
typedef struct dvm_procedure_emitter dvm_procedure_emitter;

int  dvm_proc_emitter_begin_create(dvm_procedure_emitter *procgen, dvm_context *context);

dvm_bc *dvm_proc_emitter_push_bc(size_t amount, dvm_procedure_emitter *procgen);

dvm_procedure	*dvm_proc_emitter_finalize(const char *name, uint8_t reg_count_in, uint8_t reg_count_use, uint8_t reg_count_out, dvm_procedure_emitter *procgen);
void			 dvm_proc_emitter_cancel(dvm_procedure_emitter *procgen);


#endif