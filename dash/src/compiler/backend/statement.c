#include "common.h"
#include "codegen.h"

int dcg_import_statement(
	dst_statement *statement,
	dst_proc *procedure,
	dcg_proc_decl_list *module,
	dcg_register_allocator *reg_alloc,
	dcg_bc_emitter *bc_emit,
	dsc_memory *mem
	)
{
	switch (statement->type)
	{

	case dst_statement_type_definition:
	{
		dst_id_list *cur_var = statement->definition.variables;
		dst_exp_list *cur_exp = statement->definition.values;

		if (cur_var == NULL || cur_exp == NULL)
		{
			dsc_error("invalid definition, must have at least one variable and value.");
			return 0;
		}

		do
		{
			size_t			 exp_out_reg_start;
			dst_type_list	*exp_out_types;

			// Evaluate the expression, returning n >= 1 possible values
			// n is the length of the exp_out_types linked list
			// the register storing n is exp_out_reg_start + n

			if (!dcg_import_expression(
				cur_exp->value,
				&exp_out_reg_start,
				&exp_out_types,
				module,
				reg_alloc,
				bc_emit,
				mem))
			{
				return 0;
			}

			// Cannot have a definition with zero values to assign

			if (exp_out_types == NULL)
			{
				dsc_error("invalid definition, every expression must produce a value.");
				return 0;
			}

			// Reclaim any temp registers, we'll realloc them as we need them

			if (dcg_is_temp(exp_out_reg_start, reg_alloc))
			{
				dcg_pop_temp_past(exp_out_reg_start, reg_alloc);
			}

			// Push named registers for the variables and move the values into the registers

			dst_type_list	*sub_val_type = exp_out_types;
			size_t			 sub_val_index = 0;

			while (1)
			{
				size_t val_reg = exp_out_reg_start + sub_val_index;
				size_t var_reg = dcg_push_named(cur_var->value, sub_val_type->value, reg_alloc);

				if (var_reg == ~0)
				{
					dsc_error_oor();
					return 0;
				}

				// Store the value in the variable, if it's not already there

				if (var_reg != val_reg)
				{
					dvm_bc *bc = dcg_push_bc(1, bc_emit);

					if (bc == NULL)
					{
						dsc_error_oom();
						return 0;
					}

					bc[0].opcode = dvm_opcode_mov;
					bc[0].a = exp_out_reg_start + sub_val_index;
					bc[0].c = var_reg;
				}

				// Go to the next value of this expression

				++sub_val_index;
				sub_val_type = sub_val_type->next;

				// Go to the next var of this definition

				cur_var = cur_var->next;

				// If we ran out of vars or values then exit

				if (cur_var == statement->definition.variables || sub_val_type == exp_out_types)
					break;
			}

			// Go to the next expression for its values

			cur_exp = cur_exp->next;

			// Check to see if we ran out of vars before sub vals or expressions

			if (cur_var == statement->definition.variables && (sub_val_type != exp_out_types || cur_exp != statement->definition.values))
			{
				dsc_error("invalid definition, more values than there are variables.");
				return 0;
			}

			// Check to see if we ran out of expressions before vars

			if (cur_var != statement->definition.variables && cur_exp == statement->definition.values)
			{
				dsc_error("invalid definition, less values than there are variables.");
				return 0;
			}

		} while (cur_exp != statement->definition.values);

		return 1;
	}

	case dst_statement_type_assignment:
	{
		dst_id_list *cur_var = statement->assignment.variables;
		dst_exp_list *cur_exp = statement->assignment.values;

		if (cur_var == NULL || cur_exp == NULL)
		{
			dsc_error("invalid assignment, must have at least one variable and value.");
			return 0;
		}

		do
		{
			size_t			 exp_out_reg_start;
			dst_type_list	*exp_out_types;

			// Evaluate the expression, returning n >= 1 possible values
			// n is the length of the exp_out_types linked list
			// the register storing n is exp_out_reg_start + n

			if (!dcg_import_expression(
				cur_exp->value,
				&exp_out_reg_start,
				&exp_out_types,
				module,
				reg_alloc,
				bc_emit,
				mem))
			{
				return 0;
			}
			
			// We cannot have an assignment with zero expressions in the assignment part

			if (exp_out_types == NULL)
			{
				dsc_error("invalid assignment, every expression must produce a value.");
				return 0;
			}

			// Move each sub value into it's proper variable register, checking types along the way

			dst_type_list *sub_val_type = exp_out_types;
			size_t sub_val_index = 0;

			while (1)
			{
				size_t val_reg = exp_out_reg_start + sub_val_index;
				dcg_var_binding *var = dcg_map(cur_var->value, reg_alloc);

				if (var == NULL)
				{
					dsc_error("invalid assignment, cannot find variable (%s).", cur_var->value);
					return 0;
				}

				if (var->type != sub_val_type->value)
				{
					dsc_error("invalid assignment, trying to assign the wrong type to variable.");
					return 0;
				}

				// Store the value in the variable, if it's not already there

				if (var->reg_index != val_reg)
				{
					dvm_bc *bc = dcg_push_bc(1, bc_emit);

					if (bc == NULL)
					{
						dsc_error_oom();
						return 0;
					}

					bc[0].opcode = dvm_opcode_mov;
					bc[0].a = exp_out_reg_start + sub_val_index;
					bc[0].c = var->reg_index;
				}

				// Go to the next value of this expression

				++sub_val_index;
				sub_val_type = sub_val_type->next;

				// Go to the next var of this definition

				cur_var = cur_var->next;

				if (cur_var == statement->assignment.variables || sub_val_type == exp_out_types)
					break;
			}

			// Go to the next expression for its values
			cur_exp = cur_exp->next;

			// Check to see if we ran out of vars before sub vals or expressions

			if (cur_var == statement->assignment.variables && (sub_val_type != exp_out_types || cur_exp != statement->assignment.values))
			{
				dsc_error("invalid assignment, more values than there are variables.");
				return 0;
			}

			// Check to see if we ran out of expressions before vars

			if (cur_var != statement->assignment.variables && cur_exp == statement->assignment.values)
			{
				dsc_error("invalid assignment, less values than there are variables.");
				return 0;
			}

			// Clear out the temp registers we were using

			if (dcg_is_temp(exp_out_reg_start, reg_alloc))
			{
				dcg_pop_temp_past(exp_out_reg_start, reg_alloc);
			}
			
		} while (cur_exp != statement->assignment.values);

		return 1;
	}
	
	case dst_statement_type_call:
	{
		dst_exp call_exp;

		call_exp.type = dst_exp_type_call;
		call_exp.call.function = statement->call.function;
		call_exp.call.parameters = statement->call.parameters;

		size_t			 out_reg;
		dst_type_list	*out_type;

		if (!dcg_import_expression(
			&call_exp,
			&out_reg,
			&out_type,
			module,
			reg_alloc,
			bc_emit,
			mem))
		{
			return 0;
		}

		if (dcg_is_temp(out_reg, reg_alloc))
		{
			dcg_pop_temp_past(out_reg, reg_alloc);
		}

		return 1;
	}

	case dst_statement_type_block:
	{
		size_t base_register = reg_alloc->vars_named_count;

		dst_statement_list *current = statement->block.statements;
		if (current != NULL)
		{
			do
			{
				if (!dcg_import_statement(current->value, procedure, module, reg_alloc, bc_emit, mem))
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

		if (!dcg_import_expression(statement->if_else.condition, &cond_register, &cond_type, module, reg_alloc, bc_emit, mem))
		{
			return 0;
		}

		if (!dst_type_list_is_integer(cond_type))
		{
			dsc_error("invalid if statement, conditional must be an integer.");
			return 0;
		}

		// We don't need to reserve this condition register past the first jmpc

		if (dcg_is_temp(cond_register, reg_alloc))
		{
			dcg_pop_temp_past(cond_register, reg_alloc);
		}

		// Write the jmp that will skip the true block and execute the false block

		size_t	jmp_to_false_loc = dcg_bc_written(bc_emit);
		dvm_bc *jmp_to_false = dcg_push_bc(1, bc_emit);
		if (jmp_to_false == NULL)
		{
			dsc_error_oom();
			return 0;
		}
		jmp_to_false[0].opcode = dvm_opcode_jmp_cn;
		jmp_to_false[0].a = cond_register;

		// Write the true statement

		if (!dcg_import_statement(statement->if_else.true_statement, procedure, module, reg_alloc, bc_emit, mem))
		{
			return 0;
		}

		if (statement->if_else.false_statement != NULL)
		{
			// Write the jmp to skip the false statement after the true statement

			size_t	jmp_to_end_loc = dcg_bc_written(bc_emit);
			dvm_bc *jmp_to_end = dcg_push_bc(1, bc_emit);
			if (jmp_to_end == NULL)
			{
				dsc_error_oom();
				return 0;
			}
			jmp_to_end[0].opcode = dvm_opcode_jmp_u;

			// Write the false statement

			size_t false_statement_start_loc = dcg_bc_written(bc_emit);
			if (!dcg_import_statement(statement->if_else.false_statement, procedure, module, reg_alloc, bc_emit, mem))
			{
				return 0;
			}
			size_t false_statement_end_loc = dcg_bc_written(bc_emit);

			// Resolve jmp offsets

			int8_t offset1 = (int8_t)(((int)false_statement_start_loc) - ((int)jmp_to_false_loc));
			jmp_to_false[0].c = *(uint8_t *)&offset1;

			int8_t offset2 = (int8_t)((int)false_statement_end_loc - (int)jmp_to_end_loc);
			jmp_to_end[0].c = *(uint8_t *)&offset2;
		}
		else
		{
			size_t end_loc = dcg_bc_written(bc_emit);

			// Resolve jmp offsets

			int8_t offset1 = (int8_t)(((int)end_loc) - ((int)jmp_to_false_loc));
			jmp_to_false[0].c = *(uint8_t *)&offset1;
		}

		return 1;
	}

	case dst_statement_type_while:
	{
		size_t			 cond_register;
		dst_type_list	*cond_type;

		// Write the expression for the while condition

		size_t cond_loc = dcg_bc_written(bc_emit);

		if (!dcg_import_expression(statement->while_loop.condition, &cond_register, &cond_type, module, reg_alloc, bc_emit, mem))
		{
			return 0;
		}

		if (!dst_type_list_is_integer(cond_type))
		{
			dsc_error("invalid while statement, conditional must be an integer.");
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
			dsc_error_oom();
			return 0;
		}
		jmp_break[0].opcode = dvm_opcode_jmp_cn;
		jmp_break[0].a = cond_register;

		// Write the while body

		if (!dcg_import_statement(statement->while_loop.loop_statement, procedure, module, reg_alloc, bc_emit, mem))
		{
			return 0;
		}

		// Write the jmp to go back to the conditional

		size_t	jmp_continue_loc = dcg_bc_written(bc_emit);
		dvm_bc *jmp_continue = dcg_push_bc(1, bc_emit);
		if (jmp_continue == NULL)
		{
			dsc_error_oom();
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
			dsc_error("invalid return statement, more values than procedure allows.");
			return 0;
		}

		if (cur_out != NULL && cur_exp == NULL)
		{
			dsc_error("invalid return statement, expected values.");
			return 0;
		}

		size_t start_out_register = dcg_next_reg_index(reg_alloc);

		if (cur_exp != NULL)
		{
			do
			{
				size_t			 exp_reg_start;
				dst_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dcg_import_expression(
					cur_exp->value,
					&exp_reg_start,
					&exp_types,
					module,
					reg_alloc,
					bc_emit,
					mem))
				{
					return 0;
				}

				// Cannot have an expression that yields no values

				if (exp_types == NULL)
				{
					dsc_error("invalid return statement, every expression must produce a value.");
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
						dsc_error("invalid return statement, value has different type than procedure output.");
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

					// Go to the next out of this ret

					cur_out = cur_out->next;

					if (cur_out == procedure->out_types || sub_val_type == exp_types)
						break;
				}

				// Go to the next expression for its values
				cur_exp = cur_exp->next;

				// Check to see if we ran out of outs before sub vals or expressions

				if (cur_out == procedure->out_types && (sub_val_type != exp_types || cur_exp != statement->ret.values))
				{
					dsc_error("invalid return statement, more values than procedure allows.");
					return 0;
				}

				// Check to see if we ran out of expressions before outs

				if (cur_out != procedure->out_types && cur_exp == statement->ret.values)
				{
					dsc_error("invalid return statement, expected more output values.");
					return 0;
				}
				
			} while (cur_exp != statement->ret.values);

			// Let go of the registers we've alloced for returning.
			// It's just for cleaning up after ourselves.

			dcg_pop_temp_past(start_out_register, reg_alloc);
		}

		dvm_bc *ret_bc = dcg_push_bc(1, bc_emit);

		if (ret_bc == NULL)
		{
			dsc_error_oom();
			return 0;
		}

		ret_bc[0].opcode = dvm_opcode_ret;
		ret_bc[0].a = start_out_register;

		return 1;
	}

	}

	dsc_error_internal();
	return 0;
}