#include "common.h"

int dcg_import_expression(
	dst_exp *exp,
	size_t *out_reg,
	dst_type_list **out_type,
	dcg_proc_decl_list *module,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dsc_memory *mem
	)
{
	switch (exp->type)
	{
	case dst_exp_type_variable:
	{
		dcg_var_binding	*binding = dcg_map(exp->variable.id, reg_alloc);

		if (binding == NULL)
		{
			dsc_error("couldn't find variable (%s)\n", exp->variable.id);
			return 0;
		}

		(*out_reg) = binding->reg_index;
		(*out_type) = binding->type == dst_type_real ? &dst_sentinel_type_real : &dst_sentinel_type_integer;

		return 1;
	}
	break;

	case dst_exp_type_integer:
	case dst_exp_type_real:
	{
		size_t result_reg = dcg_push_temp(reg_alloc);

		if (result_reg == ~0)
		{
			dsc_error_oor();
			return 0;
		}

		dvm_bc *bc = dcg_push_bc(2, bc_emit);

		if (bc == NULL)
		{
			dsc_error_oom();
			return 0;
		}

		bc[0].opcode = dvm_opcode_stor;
		bc[0].c = result_reg;

		if (exp->type == dst_type_integer)
		{
			*(int32_t *)(bc + 1) = exp->integer.value;
			(*out_type) = &dst_sentinel_type_integer;
		}
		else
		{
			*(float *)(bc + 1) = exp->real.value;
			(*out_type) = &dst_sentinel_type_real;
		}

		(*out_reg) = result_reg;

		return 1;
	}
	break;

	case dst_exp_type_cast:
	{
		size_t			 source_register;
		dst_type_list	*source_type;

		size_t	result_register;

		if (!dcg_import_expression(
			exp->cast.value,
			&source_register,
			&source_type,
			module,
			reg_alloc,
			bc_emit,
			mem))
		{
			return 0;
		}

		if (dst_type_list_is_composite(source_type) ||
			dst_type_list_is_integer(source_type) && exp->cast.dest_type == dst_type_integer ||
			dst_type_list_is_real(source_type) && exp->cast.dest_type == dst_type_real)
		{
			dsc_error("invalid cast expression, value must be an integer or a real, and be casted to a different type.");
			return 0;
		}

		if (dcg_is_named(source_register, reg_alloc))
		{
			result_register = dcg_push_temp(reg_alloc);

			if (result_register == ~0)
			{
				dsc_error_oor();
				return 0;
			}
		}
		else
		{
			result_register = source_register;
		}

		dvm_bc *bc = dcg_push_bc(1, bc_emit);

		if (bc == NULL)
		{
			dsc_error_oom();
			return 0;
		}

		if (exp->cast.dest_type == dst_type_real)
		{
			bc[0].opcode = dvm_opcode_castf;
			(*out_type) = &dst_sentinel_type_real;
		}
		else
		{
			bc[0].opcode = dvm_opcode_casti;
			(*out_type) = &dst_sentinel_type_integer;
		}

		bc[0].a = source_register;
		bc[0].c = result_register;

		(*out_reg) = result_register;

		return 1;
	}
	break;

	case dst_exp_type_not:
	{
		size_t			 source_register;
		dst_type_list	*source_type;

		size_t	result_register;

		if (!dcg_import_expression(
			exp->unary.value,
			&source_register,
			&source_type,
			module,
			reg_alloc,
			bc_emit,
			mem))
		{
			return 0;
		}

		if (dst_type_list_is_composite(source_type))
		{
			dsc_error("invalid not expression, value must be an integer.");
			return 0;
		}

		if (dcg_is_named(source_register, reg_alloc))
		{
			result_register = dcg_push_temp(reg_alloc);

			if (result_register == ~0)
			{
				dsc_error_oor();
				return 0;
			}
		}
		else
		{
			result_register = source_register;
		}

		dvm_bc *bc = dcg_push_bc(1, bc_emit);

		if (bc == NULL)
		{
			dsc_error_oom();
			return 0;
		}

		switch (exp->type)
		{
			case dst_exp_type_not:
			{
				if (source_type->value == dst_type_real)
				{
					dsc_error("invalid not expression, value must be an integer.");
					return 0;
				}

				bc[0].opcode = dvm_opcode_not;

				(*out_type) = &dst_sentinel_type_integer;
			}
			break;
		}

		bc[0].a = source_register;
		bc[0].c = result_register;

		(*out_reg) = result_register;

		return 1;
	}
	break;

	case dst_exp_type_addition:
	case dst_exp_type_subtraction:
	case dst_exp_type_multiplication:
	case dst_exp_type_division:
	case dst_exp_type_and:
	case dst_exp_type_or:
	case dst_exp_type_eq:
	case dst_exp_type_less:
	case dst_exp_type_less_eq:
	case dst_exp_type_greater:
	case dst_exp_type_greater_eq:
	{
		size_t			 left_exp_register;
		dst_type_list	*left_exp_type;
		size_t			 right_exp_register;
		dst_type_list	*right_exp_type;

		size_t result_register;

		if (exp->binary.left->temp_count_est >= exp->binary.right->temp_count_est)
		{
			if (!dcg_import_expression(
				exp->binary.left,
				&left_exp_register,
				&left_exp_type,
				module,
				reg_alloc,
				bc_emit,
				mem))
			{
				return 0;
			}
			if (!dcg_import_expression(
				exp->binary.right,
				&right_exp_register,
				&right_exp_type,
				module,
				reg_alloc,
				bc_emit,
				mem))
			{
				return 0;
			}
		}
		else
		{
			if (!dcg_import_expression(
				exp->binary.right,
				&right_exp_register,
				&right_exp_type,
				module,
				reg_alloc,
				bc_emit,
				mem))
			{
				return 0;
			}
			if (!dcg_import_expression(
				exp->binary.left,
				&left_exp_register,
				&left_exp_type,
				module,
				reg_alloc,
				bc_emit,
				mem))
			{
				return 0;
			}
		}

		if (dst_type_list_is_composite(left_exp_type) || dst_type_list_is_composite(right_exp_type))
		{
			dsc_error("invalid operands to binary expression, cannot be composite types");
			return 0;
		}

		if (left_exp_type->value != right_exp_type->value)
		{
			dsc_error("invalid operands to binary expression, mismatch integer, real");
			return 0;
		}

		int left_is_named = dcg_is_named(left_exp_register, reg_alloc);
		int right_is_named = dcg_is_named(right_exp_register, reg_alloc);

		if (left_is_named && right_is_named)
		{
			result_register = dcg_push_temp(reg_alloc);

			if (result_register == ~0)
			{
				dsc_error_oor();
				return 0;
			}
		}
		else if (left_is_named)
		{
			result_register = right_exp_register;
			dcg_pop_temp_to(right_exp_register, reg_alloc);
		}
		else if (right_is_named)
		{
			result_register = left_exp_register;
			dcg_pop_temp_to(left_exp_register, reg_alloc);
		}
		else
		{
			// Use the lowest temporary register we can. This is will be the one for the expression executed first.

			if (exp->binary.left->temp_count_est >= exp->binary.right->temp_count_est)
			{
				result_register = left_exp_register;
				dcg_pop_temp_to(left_exp_register, reg_alloc);
			}
			else
			{
				result_register = right_exp_register;
				dcg_pop_temp_to(right_exp_register, reg_alloc);
			}
		}

		dvm_bc *bc = dcg_push_bc(1, bc_emit);

		if (bc == NULL)
		{
			dsc_error_oom();
			return 0;
		}

		switch (exp->type)
		{
		case dst_exp_type_addition:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_addi : dvm_opcode_addf;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = left_exp_type;
			break;
		case dst_exp_type_subtraction:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_subi : dvm_opcode_subf;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = left_exp_type;
			break;
		case dst_exp_type_multiplication:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_muli : dvm_opcode_mulf;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = left_exp_type;
			break;
		case dst_exp_type_division:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_divi : dvm_opcode_divf;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = left_exp_type;
			break;

		case dst_exp_type_and:
			if (left_exp_type->value == dst_type_real || right_exp_type->value == dst_type_real)
			{
				dsc_error("invalid operands to and expression, must be of type integer");
				return 0;
			}

			bc[0].opcode = dvm_opcode_and;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = &dst_sentinel_type_integer;
			break;
			
		case dst_exp_type_or:
			if (left_exp_type->value == dst_type_real || right_exp_type->value == dst_type_real)
			{
				dsc_error("invalid operands to or expression, must be of type integer");
				return 0;
			}

			bc[0].opcode = dvm_opcode_or;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = &dst_sentinel_type_integer;
			break;

		case dst_exp_type_eq:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_cmpi_e : dvm_opcode_cmpf_e;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = &dst_sentinel_type_integer;
			break;
		case dst_exp_type_less:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_cmpi_l : dvm_opcode_cmpf_l;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = &dst_sentinel_type_integer;
			break;
		case dst_exp_type_less_eq:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_cmpi_le : dvm_opcode_cmpf_le;
			bc[0].a = left_exp_register;
			bc[0].b = right_exp_register;
			bc[0].c = result_register;

			(*out_type) = &dst_sentinel_type_integer;
			break;
		case dst_exp_type_greater:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_cmpi_l : dvm_opcode_cmpf_l;
			bc[0].a = right_exp_register;
			bc[0].b = left_exp_register;
			bc[0].c = result_register;

			(*out_type) = &dst_sentinel_type_integer;
			break;
		case dst_exp_type_greater_eq:
			bc[0].opcode = left_exp_type->value == dst_type_integer ? dvm_opcode_cmpi_le : dvm_opcode_cmpf_le;
			bc[0].a = right_exp_register;
			bc[0].b = left_exp_register;
			bc[0].c = result_register;

			(*out_type) = &dst_sentinel_type_integer;
			break;
		}

		(*out_reg) = result_register;

		return 1;
	}
	break;

	case dst_exp_type_call:
	{
		dcg_proc_decl *next_proc = dcg_proc_decl_list_find(exp->call.function, module);

		if (next_proc == NULL)
		{
			dsc_error("invalid call expression, cannot find procedure %s.", exp->call.function);
			return 0;
		}

		dst_proc_param_list *cur_param = next_proc->in_params;
		dst_exp_list *cur_param_exp = exp->call.parameters;

		if (cur_param == NULL && cur_param_exp != NULL)
		{
			dsc_error("invalid call expression, no parameters expected in procedure.");
			return 0;
		}

		if (cur_param_exp == NULL && cur_param != NULL)
		{
			dsc_error("invalid call expression, expected a parameter.");
			return 0;
		}

		size_t start_param_reg = dcg_next_reg_index(reg_alloc);

		if (cur_param_exp != NULL)
		{
			do
			{
				size_t			 exp_reg_start;
				dst_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dcg_import_expression(cur_param_exp->value, &exp_reg_start, &exp_types, module, reg_alloc, bc_emit, mem))
				{
					return 0;
				}

				if (exp_types == NULL)
				{
					dsc_error("invalid call expression, every expression must produce at least one value.");
					return 0;
				}

				// Clear out the temp registers we were using, this allows us to reclaim them with new temp variables for output

				if (dcg_is_temp(exp_reg_start, reg_alloc))
				{
					dcg_pop_temp_past(exp_reg_start, reg_alloc);
				}

				// Move the values into the output registers

				dst_type_list	*sub_val_type = exp_types;
				size_t			 sub_val_index = 0;

				while (1)
				{
					if (sub_val_type->value != cur_param->value->type)
					{
						dsc_error("invalid call expression, parameter has wrong type.");
						return 0;
					}

					size_t sub_val_reg = exp_reg_start + sub_val_index;
					size_t out_reg = dcg_push_temp(reg_alloc);

					if (out_reg == ~0)
					{
						dsc_error_oor();
						return 0;
					}

					// Store the value in the out register, if it's not already there

					if (out_reg != sub_val_reg)
					{
						dvm_bc *bc = dcg_push_bc(1, bc_emit);

						if (bc == NULL)
						{
							dsc_error_oom();
							return 0;
						}

						bc[0].opcode = dvm_opcode_mov;
						bc[0].a = sub_val_reg;
						bc[0].c = out_reg;
					}

					// Go to the next value of this expression

					++sub_val_index;
					sub_val_type = sub_val_type->next;

					// Go to the next param of this call

					cur_param = cur_param->next;

					if (cur_param == next_proc->in_params || sub_val_type == exp_types)
						break;
				}

				// Go to the next expression for its values
				cur_param_exp = cur_param_exp->next;

				// Check to see if we ran out of params before sub vals or expressions

				if (cur_param == next_proc->in_params && (sub_val_type != exp_types || cur_param_exp != exp->call.parameters))
				{
					dsc_error("invalid call expression, too many parameters.");
					return 0;
				}

				// Check to see if we ran out of expression before params

				if (cur_param != next_proc->in_params && cur_param_exp == exp->call.parameters)
				{
					dsc_error("invalid call expression, not enough parameters.");
					return 0;
				}

			} while (cur_param_exp != exp->call.parameters);
		}

		dvm_bc *call = dcg_push_bc(1, bc_emit);

		if (call == NULL)
		{
			dsc_error_oom();
			return 0;
		}

		call->opcode = dvm_opcode_call;
		call->a = next_proc->index;
		call->b = start_param_reg;
		call->c = start_param_reg;

		dcg_pop_temp_past(start_param_reg, reg_alloc);

		size_t out_val_count = dst_type_list_count(next_proc->out_types);

		while (out_val_count > 0)
		{
			dcg_push_temp(reg_alloc);
			--out_val_count;
		}

		(*out_type) = next_proc->out_types;
		(*out_reg) = start_param_reg;

		return 1;
	}

	}

	dsc_error_internal();
	return 0;
}