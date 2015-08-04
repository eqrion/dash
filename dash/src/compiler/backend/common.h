#ifndef dash_compiler_backend_common_h
#define dash_compiler_backend_common_h

#include "../../hash.h"
#include "../../vm_internal.h"
#include "../ast.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

struct dcg_var_binding
{
	uint32_t		hashed_name;
	size_t			reg_index;
	dst_type		type;
};
struct dcg_register_allocator
{
	size_t					vars_named_count;
	size_t					vars_temp_count;
	size_t					vars_max_allocated;

	size_t					named_vars_capacity;
	struct dcg_var_binding *named_vars;
};
struct dcg_bc_emitter
{
	dvm_procedure_emitter vm_emitter;
};
typedef struct dcg_var_binding dcg_var_binding;
typedef struct dcg_register_allocator dcg_register_allocator;
typedef struct dcg_bc_emitter dcg_bc_emitter;

inline int get_error_code()
{
	/* ;) */
	return (rand() % 2000) + 2555;
}

int	dcg_start_proc_emit(
	size_t initial_var_capacity,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dvm_context *vm);
int dcg_finalize_proc_emit(
	dst_proc *ast_proc,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dvm_context *vm);
void dcg_cancel_proc_emit(
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dvm_context *vm);

dcg_var_binding *dcg_map(const char *name, dcg_register_allocator *reg_alloc);

size_t	dcg_push_temp(dcg_register_allocator *reg_alloc);
void	dcg_pop_temp_to(size_t temp_reg_index, dcg_register_allocator *reg_alloc);
void	dcg_pop_temp_past(size_t temp_reg_index, dcg_register_allocator *reg_alloc);

size_t	dcg_push_named(const char *name, dst_type type, dcg_register_allocator *reg_alloc);
void	dcg_pop_named_to(size_t named_reg_index, dcg_register_allocator *reg_alloc);
void	dcg_pop_named_past(size_t named_reg_index, dcg_register_allocator *reg_alloc);

int		dcg_is_named(size_t reg_index, dcg_register_allocator *reg_alloc);
int		dcg_is_temp(size_t reg_index, dcg_register_allocator *reg_alloc);

dvm_bc  *dcg_push_bc(size_t amount, dcg_bc_emitter *bc_emit);

size_t dcg_bc_written(dcg_bc_emitter *bc_emit);

#endif