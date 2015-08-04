#include "common.h"

int dcg_import_expression(
	dst_exp *exp,
	size_t *out_reg,
	dst_type_list **out_type,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit
	)
{
	switch (exp->type)
	{
	case dst_exp_type_variable:
	{
		dcg_var_binding	*binding = dcg_map(exp->variable.id, reg_alloc);

		if (binding == NULL)
		{
			fprintf(stderr, "error dsc%i: invalid variable identifier (%s)\n", get_error_code(), exp->variable.id);
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
			fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
			return 0;
		}

		dvm_bc *bc = dcg_push_bc(2, bc_emit);

		if (bc == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
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
			reg_alloc,
			bc_emit))
		{
			return 0;
		}

		if (dst_type_list_is_composite(source_type) ||
			dst_type_list_is_integer(source_type) && exp->cast.dest_type == dst_type_integer ||
			dst_type_list_is_real(source_type) && exp->cast.dest_type == dst_type_real)
		{
			fprintf(stderr, "error dsc%i: invalid cast expression\n", get_error_code());
			return 0;
		}

		if (dcg_is_named(source_register, reg_alloc))
		{
			result_register = dcg_push_temp(reg_alloc);

			if (result_register == ~0)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
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
			fprintf(stderr, "error dsc%i: internal error, cannot bytecode \n", get_error_code());
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

	case dst_exp_type_addition:
	case dst_exp_type_subtraction:
	case dst_exp_type_multiplication:
	case dst_exp_type_division:
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

		if (exp->operator.left->temp_count_est >= exp->operator.right->temp_count_est)
		{
			if (!dcg_import_expression(
				exp->operator.left,
				&left_exp_register,
				&left_exp_type,
				reg_alloc,
				bc_emit))
			{
				return 0;
			}
			if (!dcg_import_expression(
				exp->operator.right,
				&right_exp_register,
				&right_exp_type,
				reg_alloc,
				bc_emit))
			{
				return 0;
			}
		}
		else
		{
			if (!dcg_import_expression(
				exp->operator.right,
				&right_exp_register,
				&right_exp_type,
				reg_alloc,
				bc_emit))
			{
				return 0;
			}
			if (!dcg_import_expression(
				exp->operator.left,
				&left_exp_register,
				&left_exp_type,
				reg_alloc,
				bc_emit))
			{
				return 0;
			}
		}

		if (dst_type_list_is_composite(left_exp_type) || dst_type_list_is_composite(right_exp_type))
		{
			fprintf(stderr, "error dsc%i: invalid operands to binary expression, cannot be composite types\n", get_error_code());
			return 0;
		}

		if (left_exp_type->value != right_exp_type->value)
		{
			fprintf(stderr, "error dsc%i: invalid operands to binary expression, mismatch integer, real\n", get_error_code());
			return 0;
		}

		int left_is_named = dcg_is_named(left_exp_register, reg_alloc);
		int right_is_named = dcg_is_named(right_exp_register, reg_alloc);

		if (left_is_named && right_is_named)
		{
			result_register = dcg_push_temp(reg_alloc);

			if (result_register == ~0)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
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

			if (exp->operator.left->temp_count_est >= exp->operator.right->temp_count_est)
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
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
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
		fprintf(stderr, "error dsc%i: calls are not implemented\n", get_error_code());
		return 0;
	}
	break;
	}

	fprintf(stderr, "error dsc%i: internal error, invalid expression in ast\n", get_error_code());
	return 0;
}