#include "common.h"

dcg_proc_decl_list *dcg_append_proc_decl_list(dcg_proc_decl_list *list, dcg_proc_decl *value, dsc_memory *mem)
{
	if (list == NULL)
	{
		list = (dcg_proc_decl_list *)dsc_alloc(sizeof(dcg_proc_decl_list), mem);

		if (list == NULL)
			return NULL;

		list->next = list;
		list->prev = list;
		list->value = value;
	}
	else
	{
		dcg_proc_decl_list *next = (dcg_proc_decl_list *)dsc_alloc(sizeof(dcg_proc_decl_list), mem);

		if (next == NULL)
			return NULL;

		next->value = value;
		next->next = list;
		next->prev = list->prev;

		list->prev->next = next;
		list->prev = next;
	}

	return list;
}

dcg_proc_decl *dcg_create_proc_decl(const char *id, dst_proc_param_list *in_params, dst_type_list *out_types, size_t index, dsc_memory *mem)
{
	dcg_proc_decl *value = (dcg_proc_decl *)dsc_alloc(sizeof(dcg_proc_decl), mem);

	if (value == NULL)
	{
		return NULL;
	}

	value->id = id;
	value->index = index;
	value->in_params = in_params;
	value->out_types = out_types;

	return value;
}

dcg_proc_decl *dcg_proc_decl_list_find(const char *id, dcg_proc_decl_list *list)
{
	dcg_proc_decl_list *current = list;

	if (current == NULL)
	{
		return NULL;
	}

	do
	{
		if (strcmp(current->value->id, id) == 0)
		{
			return current->value;
		}

		current = current->next;
	} while (current != list);

	return NULL;
}

int	dcg_start_proc_emit(
	size_t initial_var_capacity,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dvm_context *vm,
	dsc_memory *mem)
{
	reg_alloc->vars_named_count = 0;
	reg_alloc->vars_temp_count = 0;
	reg_alloc->vars_max_allocated = 0;

	reg_alloc->named_vars_capacity = initial_var_capacity == 0 ? 4 : initial_var_capacity;
	reg_alloc->named_vars = (dcg_var_binding *)dsc_alloc(sizeof(dcg_var_binding) * reg_alloc->named_vars_capacity, mem);

	reg_alloc->mem = mem;

	if (reg_alloc->named_vars == NULL)
	{
		return 0;
	}
	
	if (!dvm_proc_emitter_begin_create(&bc_emit->vm_emitter, vm))
	{
		return 0;
	}

	return 1;
}
int dcg_finalize_proc_emit(
	dst_proc *ast_proc,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dvm_context *vm)
{
	size_t in_count = dst_proc_param_list_count(ast_proc->in_params);
	size_t out_count = dst_type_list_count(ast_proc->out_types);

	dvm_procedure *result = dvm_proc_emitter_finalize(
		ast_proc->id,
		(uint8_t)in_count,
		(uint8_t)(reg_alloc->vars_max_allocated - in_count),
		(uint8_t)out_count,
		&bc_emit->vm_emitter);
	
	return result != NULL;
}
void dcg_cancel_proc_emit(
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dvm_context *vm)
{
	dvm_proc_emitter_cancel(&bc_emit->vm_emitter);
}

dcg_var_binding *dcg_map(const char *name, dcg_register_allocator *reg_alloc)
{
	uint32_t hashed_name = dsh_hash(name);

	for (size_t i = 0; i < reg_alloc->vars_named_count; ++i)
	{
		if (reg_alloc->named_vars[i].hashed_name == hashed_name)
		{
			return &reg_alloc->named_vars[i];
		}
	}

	return NULL;
}

size_t	dcg_push_temp(dcg_register_allocator *reg_alloc)
{
	if (reg_alloc->vars_named_count + reg_alloc->vars_temp_count >= 255)
		return ~0;

	size_t result = (reg_alloc->vars_temp_count++) + reg_alloc->vars_named_count;

	if (reg_alloc->vars_max_allocated < reg_alloc->vars_temp_count + reg_alloc->vars_named_count)
		reg_alloc->vars_max_allocated = reg_alloc->vars_temp_count + reg_alloc->vars_named_count;

	return result;
}
void	dcg_pop_temp_to(size_t temp_reg_index, dcg_register_allocator *reg_alloc)
{
	assert(temp_reg_index >= reg_alloc->vars_named_count);
	assert(temp_reg_index <= reg_alloc->vars_named_count + reg_alloc->vars_temp_count);

	reg_alloc->vars_temp_count = temp_reg_index - reg_alloc->vars_named_count + 1;
}
void	dcg_pop_temp_past(size_t temp_reg_index, dcg_register_allocator *reg_alloc)
{
	assert(temp_reg_index >= reg_alloc->vars_named_count);
	assert(temp_reg_index <= reg_alloc->vars_named_count + reg_alloc->vars_temp_count);

	reg_alloc->vars_temp_count = temp_reg_index - reg_alloc->vars_named_count;
}

size_t	dcg_push_named(const char *name, dst_type type, dcg_register_allocator *reg_alloc)
{
	assert(reg_alloc->vars_temp_count == 0);

	if (reg_alloc->vars_named_count >= 255)
	{
		return ~0;
	}

	if (dcg_map(name, reg_alloc) != NULL)
	{
		return ~0;
	}

	if (reg_alloc->vars_named_count == reg_alloc->named_vars_capacity)
	{
		reg_alloc->named_vars_capacity *= 2;

		dcg_var_binding *new_stack = (dcg_var_binding *)dsc_alloc(sizeof(dcg_var_binding) * reg_alloc->named_vars_capacity, reg_alloc->mem);

		if (new_stack == NULL)
		{
			return ~0;
		}

		memcpy(new_stack, reg_alloc->named_vars, sizeof(dcg_var_binding) * reg_alloc->vars_named_count);

		reg_alloc->named_vars = new_stack;
	}
	
	reg_alloc->named_vars[reg_alloc->vars_named_count].hashed_name = dsh_hash(name);
	reg_alloc->named_vars[reg_alloc->vars_named_count].reg_index = reg_alloc->vars_named_count;
	reg_alloc->named_vars[reg_alloc->vars_named_count].type = type;

	return reg_alloc->vars_named_count++;
}
void	dcg_pop_named_to(size_t named_reg_index, dcg_register_allocator *reg_alloc)
{
	assert(named_reg_index <= reg_alloc->vars_named_count);
	assert(reg_alloc->vars_temp_count == 0);

	reg_alloc->vars_named_count = named_reg_index + 1;
}
void	dcg_pop_named_past(size_t named_reg_index, dcg_register_allocator *reg_alloc)
{
	assert(named_reg_index <= reg_alloc->vars_named_count);
	assert(reg_alloc->vars_temp_count == 0);

	reg_alloc->vars_named_count = named_reg_index;
}

int		dcg_is_named(size_t reg_index, dcg_register_allocator *reg_alloc)
{
	return reg_index < reg_alloc->vars_named_count;
}
int		dcg_is_temp(size_t reg_index, dcg_register_allocator *reg_alloc)
{
	return reg_index >= reg_alloc->vars_named_count;
}

dvm_bc  *dcg_push_bc(size_t amount, dcg_bc_emitter *bc_emit)
{
	return dvm_proc_emitter_push_bc(amount, &bc_emit->vm_emitter);
}

size_t dcg_next_reg_index(dcg_register_allocator *reg_alloc)
{
	return reg_alloc->vars_named_count + reg_alloc->vars_temp_count;
}
size_t dcg_bc_written(dcg_bc_emitter *bc_emit)
{
	return bc_emit->vm_emitter.bytecode_allocated;
}