#include "common.h"
#include "codegen.h"

dcg_proc_decl_list *dcg_import_proc_decls(dst_proc_list *procs, dcg_proc_decl_list *list, dvm_context *context, dsc_memory *mem)
{
	dst_proc_list *current = procs;

	size_t base_index = context->function_count;

	while (current != NULL)
	{
		dcg_proc_decl *decl = (dcg_proc_decl *)dsc_alloc(sizeof(dcg_proc_decl), mem);

		if (decl == NULL)
		{
			dsc_error_oom();
			break;
		}

		decl->id = current->value->id;
		decl->index = base_index;
		decl->in_params = current->value->in_params;
		decl->out_types = current->value->out_types;

		list = dcg_append_proc_decl_list(list, decl, mem);

		current = current->next;
		++base_index;

		if (current == procs)
			break;
	}

	return list;
}

int dcg_import_procedure_list(
	dst_proc_list *list,
	dcg_proc_decl_list *module,
	dvm_context *vm,
	dsc_memory *mem
	)
{
	module = dcg_import_proc_decls(list, module, vm, mem);

	dst_proc_list *current = list;

	while (current != NULL)
	{
		if (!dcg_import_procedure(current->value, module, vm, mem))
		{
			return 0;
		}

		current = current->next;

		if (current == list)
			break;
	}

	return 1;
}

int dcg_import_procedure_params(
	dst_proc_param_list *params,
	dcg_register_allocator *reg_alloc,
	dsc_memory *mem
	)
{
	if (params == NULL)
		return 1;

	dst_proc_param_list *current = params;

	do
	{
		if (dcg_push_named(current->value->id, current->value->type, reg_alloc) == ~0)
		{
			dsc_error_oor();
			return 0;
		}

		current = current->next;

	} while (current != params);

	return 1;
}

int dcg_import_procedure(
	dst_proc *proc,
	dcg_proc_decl_list *module,
	dvm_context *vm,
	dsc_memory *mem
	)
{
	size_t in_count = dst_proc_param_list_count(proc->in_params);
	size_t out_count = dst_type_list_count(proc->out_types);

	if (in_count > 255 || out_count > 255)
	{
		dsc_error("invalid function, too many parameters or outputs.");
		return 0;
	}

	dcg_register_allocator reg_alloc;
	dcg_bc_emitter bc_emit;

	if (!dcg_start_proc_emit(16, &reg_alloc, &bc_emit, vm, mem))
	{
		dsc_error_oom();
		return 0;
	}

	if (!dcg_import_procedure_params(proc->in_params, &reg_alloc, mem))
	{
		dcg_cancel_proc_emit(&reg_alloc, &bc_emit, vm);

		return 0;
	}

	if (!dcg_import_statement(proc->statement, proc, module, &reg_alloc, &bc_emit, mem))
	{
		dcg_cancel_proc_emit(&reg_alloc, &bc_emit, vm);

		return 0;
	}

	if (!dcg_finalize_proc_emit(
		proc,
		&reg_alloc,
		&bc_emit,
		vm))
	{
		dsc_error("couldn't finalize function.");
		return 0;
	}
	
	return 1;
}