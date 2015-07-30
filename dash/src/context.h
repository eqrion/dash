#ifndef dsh_context_h
#define dsh_context_h

#include <stdint.h>

#include "dash/var.h"
#include "dash/context.h"
#include "bytecode.h"

typedef void (*dsh_c_function)(const dsh_var *in_registers, dsh_var *out_registers);

struct dsh_function_def
{
	uint32_t hashed_name;

	uint8_t reg_count_in;
	uint8_t reg_count_use;
	uint8_t reg_count_out;

	dsh_c_function c_function;

	uint32_t bytecode_start;
	uint32_t bytecode_end;
};
typedef struct dsh_function_def dsh_function_def;

struct dsh_context
{
	uint32_t					 function_capacity;
	uint32_t					 function_count;
	dsh_function_def			*function;

	uint32_t	 bytecode_capacity;
	uint32_t	 bytecode_count;
	dsh_bc		*bytecode;
};
typedef struct dsh_context dsh_context;

dsh_bc				*dsh_context_push_bytecode(size_t amount, dsh_context *context);
dsh_function_def	*dsh_context_push_function(size_t amount, dsh_context *context);

void dsh_context_pop_bytecode(size_t amount, dsh_context *context);
void dsh_context_pop_function(size_t amount, dsh_context *context);

#endif