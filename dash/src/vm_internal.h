#ifndef dsh_vm_internal_h
#define dsh_vm_internal_h

#include <stdint.h>

#include "dash/var.h"
#include "dash/vm.h"

enum dvm_opcode
{
	dvm_opcode_nop = 0,

	dvm_opcode_call,
	dvm_opcode_ret,
	dvm_opcode_mov,
	dvm_opcode_stor,

	dvm_opcode_and,
	dvm_opcode_or,
	dvm_opcode_not,

	dvm_opcode_cmpi_e,
	dvm_opcode_cmpf_e,

	dvm_opcode_cmpi_l,
	dvm_opcode_cmpf_l,

	dvm_opcode_cmpi_le,
	dvm_opcode_cmpf_le,

	dvm_opcode_jmp_c,
	dvm_opcode_jmp_cn,
	dvm_opcode_jmp_u,
	
	dvm_opcode_addi,
	dvm_opcode_addf,

	dvm_opcode_subi,
	dvm_opcode_subf,

	dvm_opcode_muli,
	dvm_opcode_mulf,

	dvm_opcode_divi,
	dvm_opcode_divf,

	dvm_opcode_casti,
	dvm_opcode_castf,
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