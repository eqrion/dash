#include "common.h"
#include "codegen.h"

int dcg_import_statement(
	dst_statement *statement,
	dst_proc *procedure,
	dst_proc_list *module,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit
	)
{
	switch (statement->type)
	{

	case dst_statement_type_definition:
	{
		dst_id_list *cur_var = statement->definition.variables;
		dst_exp_list *cur_exp = statement->definition.values;

		if (cur_var != NULL && cur_exp != NULL)
		{
			do
			{
				size_t			 exp_reg_start;
				dst_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dcg_import_expression(cur_exp->value, &exp_reg_start, &exp_types, module, reg_alloc, bc_emit))
				{
					return 0;
				}

				// Move the values into the variable registers
				// and define the variables in reg_alloc with the proper type

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid definition, cannot have an expression with values\n", get_error_code());
					return 0;
				}

				// Reclaim the temp registers for when we formally push temp variables later

				if (dcg_is_temp(exp_reg_start, reg_alloc))
				{
					dcg_pop_temp_past(exp_reg_start, reg_alloc);
				}

				dst_type_list	*sub_val_type = exp_types;
				size_t			 sub_val_index = 0;

				while (1)
				{
					size_t val_reg = exp_reg_start + sub_val_index;
					size_t var_reg = dcg_push_named(cur_var->value, sub_val_type->value, reg_alloc);

					if (var_reg == ~0)
					{
						fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
						return 0;
					}

					// Store the value in the variable, if it's not already there

					if (var_reg != val_reg)
					{
						dvm_bc *bc = dcg_push_bc(1, bc_emit);

						if (bc == NULL)
						{
							fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
							return 0;
						}

						bc[0].opcode = dvm_opcode_mov;
						bc[0].a = exp_reg_start + sub_val_index;
						bc[0].c = var_reg;
					}

					// Go to the next value of this expression

					++sub_val_index;
					sub_val_type = sub_val_type->next;

					// Go to the next var of this definition

					cur_var = cur_var->next;

					if (cur_var == statement->definition.variables || sub_val_type == exp_types)
						break;
				}

				// Check to see if we ran out of vars before sub vals or expressions

				if (cur_var == statement->definition.variables && (sub_val_type != exp_types || cur_exp != statement->definition.values))
				{
					fprintf(stderr, "error dsc%i: invalid definition, too many values\n", get_error_code());
					return 0;
				}

				// Check to see if we ran out of expressions before vars

				if (cur_var != statement->definition.variables && cur_exp == statement->definition.values)
				{
					fprintf(stderr, "error dsc%i: invalid definition, not enough values\n", get_error_code());
					return 0;
				}

				// Go to the next expression for its values
				cur_exp = cur_exp->next;

			} while (cur_exp != statement->definition.values);

			return 1;
		}

		fprintf(stderr, "error dsc%i: invalid definition, zero ids or exps\n", get_error_code());
		return 0;
	}

	case dst_statement_type_assignment:
	{
		dst_id_list *cur_var = statement->assignment.variables;
		dst_exp_list *cur_exp = statement->assignment.values;

		if (cur_var != NULL && cur_exp != NULL)
		{
			do
			{
				size_t			 exp_reg_start;
				dst_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dcg_import_expression(cur_exp->value, &exp_reg_start, &exp_types, module, reg_alloc, bc_emit))
				{
					return 0;
				}

				// Move the values into the variable registers
				// and define the variables in reg_alloc with the proper type

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid assignment, cannot have an expression with zero values\n", get_error_code());
					return 0;
				}

				dst_type_list *sub_val_type = exp_types;
				size_t sub_val_index = 0;

				while (1)
				{
					size_t val_reg = exp_reg_start + sub_val_index;
					dcg_var_binding *var = dcg_map(cur_var->value, reg_alloc);

					if (var == NULL)
					{
						fprintf(stderr, "error dsc%i: invalid assignment, invalid variable identifier (%s)\n", get_error_code(), cur_var->value);
						return 0;
					}

					if (var->type != sub_val_type->value)
					{
						fprintf(stderr, "error dsc%i: invalid assignment, value mismatches variable\n", get_error_code());
						return 0;
					}

					// Store the value in the variable, if it's not already there

					if (var->reg_index != val_reg)
					{
						dvm_bc *bc = dcg_push_bc(1, bc_emit);

						if (bc == NULL)
						{
							fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
							return 0;
						}

						bc[0].opcode = dvm_opcode_mov;
						bc[0].a = exp_reg_start + sub_val_index;
						bc[0].c = var->reg_index;
					}

					// Go to the next value of this expression

					++sub_val_index;
					sub_val_type = sub_val_type->next;

					// Go to the next var of this definition

					cur_var = cur_var->next;

					if (cur_var == statement->assignment.variables || sub_val_type == exp_types)
						break;
				}

				// Check to see if we ran out of vars before sub vals or expressions

				if (cur_var == statement->assignment.variables && (sub_val_type != exp_types || cur_exp != statement->assignment.values))
				{
					fprintf(stderr, "error dsc%i: invalid assignment, not enough vars\n", get_error_code());
					return 0;
				}

				// Check to see if we ran out of expressions before vars

				if (cur_var != statement->assignment.variables && cur_exp == statement->assignment.values)
				{
					fprintf(stderr, "error dsc%i: invalid assignment, not enough values\n", get_error_code());
					return 0;
				}

				// Clear out the temp registers we were using

				if (dcg_is_temp(exp_reg_start, reg_alloc))
				{
					dcg_pop_temp_past(exp_reg_start, reg_alloc);
				}
				
				// Go to the next expression for its values
				cur_exp = cur_exp->next;
				
			} while (cur_exp != statement->assignment.values);

			return 1;
		}

		fprintf(stderr, "error dsc%i: invalid assignment, zero ids or exps\n", get_error_code());
		return 0;
	}
	
	case dst_statement_type_block:
	{
		size_t base_register = reg_alloc->vars_named_count;

		dst_statement_list *current = statement->block.statements;
		if (current != NULL)
		{
			do
			{
				if (!dcg_import_statement(current->value, procedure, module, reg_alloc, bc_emit))
				{
					return 0;
				}

				current = current->next;
			} while (current != statement->block.statements);
		}

		dcg_pop_named_past(base_register, reg_alloc);

		return 1;
	}

	case dst_statement_type_if:
	{
		size_t			 cond_register;
		dst_type_list	*cond_type;

		// Write the if conditional first

		if (!dcg_import_expression(statement->if_else.condition, &cond_register, &cond_type, module, reg_alloc, bc_emit))
		{
			return 0;
		}

		if (!dst_type_list_is_integer(cond_type))
		{
			fprintf(stderr, "error dsc%i: invalid if statement, conditional must be an integer\n", get_error_code());
			return 0;
		}

		// We don't need to reserve this condition register past the first jmpc

		if (dcg_is_temp(cond_register, reg_alloc))
		{
			dcg_pop_temp_past(cond_register, reg_alloc);
		}

		// Write the jmp that will skip the false block and execute the true block

		size_t	jmp_to_true_loc = dcg_bc_written(bc_emit);
		dvm_bc *jmp_to_true = dcg_push_bc(1, bc_emit);
		if (jmp_to_true == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_to_true[0].opcode = dvm_opcode_jmp_c;
		jmp_to_true[0].a = cond_register;

		// Write the false statement

		if (!dcg_import_statement(statement->if_else.false_statement, procedure, module, reg_alloc, bc_emit))
		{
			return 0;
		}

		// Write the jmp to skip the true statement after the false statement

		size_t	jmp_to_end_loc = dcg_bc_written(bc_emit);
		dvm_bc *jmp_to_end = dcg_push_bc(1, bc_emit);
		if (jmp_to_end == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_to_end[0].opcode = dvm_opcode_jmp_u;

		// Write the true statement

		size_t true_statement_start_loc = dcg_bc_written(bc_emit);
		if (!dcg_import_statement(statement->if_else.true_statement, procedure, module, reg_alloc, bc_emit))
		{
			return 0;
		}
		size_t true_statement_end_loc = dcg_bc_written(bc_emit);

		// Resolve jmp offsets

		int8_t offset1 = (int8_t)(((int)true_statement_start_loc) - ((int)jmp_to_true_loc));
		jmp_to_true[0].c = *(uint8_t *)&offset1;

		int8_t offset2 = (int8_t)((int)true_statement_end_loc - (int)jmp_to_end_loc);
		jmp_to_end[0].c = *(uint8_t *)&offset2;

		return 1;
	}

	case dst_statement_type_while:
	{
		size_t			 cond_register;
		dst_type_list	*cond_type;

		// Write the expression for the while condition

		size_t cond_loc = dcg_bc_written(bc_emit);

		if (!dcg_import_expression(statement->while_loop.condition, &cond_register, &cond_type, module, reg_alloc, bc_emit))
		{
			return 0;
		}

		if (!dst_type_list_is_integer(cond_type))
		{
			fprintf(stderr, "error dsc%i: invalid while statement, conditional must be an integer\n", get_error_code());
			return 0;
		}

		// We don't need to reserve this condition register after the jmpc

		if (dcg_is_temp(cond_register, reg_alloc))
		{
			dcg_pop_temp_past(cond_register, reg_alloc);
		}

		// Write the jmp to skip the block

		size_t	jmp_break_loc = dcg_bc_written(bc_emit);
		dvm_bc *jmp_break = dcg_push_bc(1, bc_emit);
		if (jmp_break == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_break[0].opcode = dvm_opcode_jmp_cn;
		jmp_break[0].a = cond_register;

		// Write the while body

		if (!dcg_import_statement(statement->while_loop.loop_statement, procedure, module, reg_alloc, bc_emit))
		{
			return 0;
		}

		// Write the jmp to go back to the conditional

		size_t	jmp_continue_loc = dcg_bc_written(bc_emit);
		dvm_bc *jmp_continue = dcg_push_bc(1, bc_emit);
		if (jmp_continue == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_continue[0].opcode = dvm_opcode_jmp_u;

		// Resolve the jmp offsets

		int8_t offset1 = (int8_t)((int)cond_loc - (int)jmp_continue_loc);
		jmp_continue[0].c = *(uint8_t *)&offset1;

		int8_t offset2 = (int8_t)((int)jmp_continue_loc + 1 - (int)jmp_break_loc);
		jmp_break[0].c = *(uint8_t *)&offset2;

		return 1;
	}

	case dst_statement_type_return:
	{
		dst_type_list *cur_out = procedure->out_types;
		dst_exp_list *cur_exp = statement->ret.values;

		if (cur_out == NULL && cur_exp != NULL)
		{
			fprintf(stderr, "error dsc%i: invalid return statement, no out values expected in function\n", get_error_code());
			return 0;
		}

		size_t last_out_register = 0;
		size_t out_count = 0;

		if (cur_out == NULL && cur_exp != NULL)
		{
			fprintf(stderr, "error dsc%i: invalid return statement, expected no out values\n", get_error_code());
			return 0;
		}

		if (cur_exp != NULL)
		{
			do
			{
				size_t			 exp_reg_start;
				dst_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dcg_import_expression(cur_exp->value, &exp_reg_start, &exp_types, module, reg_alloc, bc_emit))
				{
					return 0;
				}

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid return statement, cannot have an expression with zero values\n", get_error_code());
					return 0;
				}

				// Reclaim the temp registers for when we formally push temp variables later

				if (dcg_is_temp(exp_reg_start, reg_alloc))
				{
					dcg_pop_temp_past(exp_reg_start, reg_alloc);
				}

				// Move the values into the output registers

				dst_type_list	*sub_val_type = exp_types;
				size_t			 sub_val_index = 0;

				while (1)
				{
					if (sub_val_type->value != cur_out->value)
					{
						fprintf(stderr, "error dsc%i: invalid return statement, out type mismatch\n", get_error_code());
						return 0;
					}

					size_t sub_val_reg = exp_reg_start + sub_val_index;
					size_t out_reg = dcg_push_temp(reg_alloc);

					if (out_reg == ~0)
					{
						fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
						return 0;
					}

					last_out_register = out_reg;

					// Store the value in the out register, if it's not already there

					if (out_reg != sub_val_reg)
					{
						dvm_bc *bc = dcg_push_bc(1, bc_emit);

						if (bc == NULL)
						{
							fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
							return 0;
						}

						bc[0].opcode = dvm_opcode_mov;
						bc[0].a = sub_val_reg;
						bc[0].c = out_reg;
					}

					// Go to the next value of this expression

					++sub_val_index;
					sub_val_type = sub_val_type->next;

					// Go to the next out of this ret

					cur_out = cur_out->next;
					++out_count;

					if (cur_out == procedure->out_types || sub_val_type == exp_types)
						break;
				}

				// Check to see if we ran out of outs before sub vals or expressions

				if (cur_out == procedure->out_types && (sub_val_type != exp_types || cur_exp != statement->ret.values))
				{
					fprintf(stderr, "error dsc%i: invalid return statement, too many out values\n", get_error_code());
					return 0;
				}

				// Check to see if we ran out of expressions before outs

				if (cur_out != procedure->out_types && cur_exp == statement->ret.values)
				{
					fprintf(stderr, "error dsc%i: invalid return statement, not enough out values\n", get_error_code());
					return 0;
				}

				// Go to the next expression for its values
				cur_exp = cur_exp->next;


			} while (cur_exp != statement->ret.values);

			dvm_bc *ret_bc = dcg_push_bc(1, bc_emit);

			if (ret_bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}

			ret_bc[0].opcode = dvm_opcode_ret;
			ret_bc[0].a = last_out_register - (out_count - 1);

			dcg_pop_temp_past(last_out_register - (out_count - 1), reg_alloc);

			return 1;
		}
		else
		{
			dvm_bc *ret_bc = dcg_push_bc(1, bc_emit);

			if (ret_bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}

			ret_bc[0].opcode = dvm_opcode_ret;

			dcg_pop_temp_past(last_out_register - (out_count - 1), reg_alloc);

			return 1;
		}

	}

	}

	fprintf(stderr, "error dsc%i: internal error, invalid statement in ast\n", get_error_code());
	return 0;
}