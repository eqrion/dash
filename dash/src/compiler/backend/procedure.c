#include "common.h"
#include "codegen.h"

int dcg_import_procedure_list(
	dst_proc_list *list,
	dvm_context *vm
	)
{
	dst_proc_list *current = list;

	while (current != NULL)
	{
		if (!dcg_import_procedure(current->value, vm))
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
	dcg_register_allocator *reg_alloc
	)
{
	if (params == NULL)
		return 1;

	dst_proc_param_list *current = params;

	do
	{
		if (dcg_push_named(current->value->id, current->value->type, reg_alloc) == ~0)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
			return 0;
		}

		current = current->next;

	} while (current != params);

	return 1;
}

int dcg_import_procedure(
	dst_proc *proc,
	dvm_context *vm
	)
{
	size_t in_count = dst_proc_param_list_count(proc->in_params);
	size_t out_count = dst_type_list_count(proc->out_types);

	if (in_count > 255 || out_count > 255)
	{
		fprintf(stderr, "error dsc%i: invalid function, too many params or outs\n", get_error_code());
		return 0;
	}

	dcg_register_allocator reg_alloc;
	dcg_bc_emitter bc_emit;

	if (!dcg_start_proc_emit(16, &reg_alloc, &bc_emit, vm))
	{
		fprintf(stderr, "error dsc%i: internal error, cannot allocate internal memory\n", get_error_code());

		return 0;
	}

	if (!dcg_import_procedure_params(proc->in_params, &reg_alloc))
	{
		dcg_cancel_proc_emit(&reg_alloc, &bc_emit, vm);

		return 0;
	}

	if (!dcg_import_statement(proc->statement, proc, &reg_alloc, &bc_emit))
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
		return 0;
	}
	
	return 1;
}