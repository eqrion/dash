#include "../hash.h"
#include "../vm_internal.h"
#include "ast.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <time.h>

int dst_type_list_is_integer(dst_type_list *list)
{
	return (list->value == dst_type_integer) && (list->next == list);
}
int dst_type_list_is_real(dst_type_list *list)
{
	return (list->value == dst_type_real) && (list->next == list);
}
int dst_type_list_is_composite(dst_type_list *list)
{
	return list->next != list;
}

struct dcg_var_binding
{
	uint32_t		hashed_name;
	size_t			reg_index;
	dst_type		type;
};
struct dcg_proc_context
{
	size_t					vars_named_count;
	size_t					vars_temp_count;
	size_t					vars_max_allocated;

	size_t					named_vars_capacity;
	struct dcg_var_binding *named_vars;

	dst_func				*function;
	dvm_procedure_generator  procgen;
	dvm_context				*context;
};
typedef struct dcg_var_binding dcg_var_binding;
typedef struct dcg_proc_context dcg_proc_context;

int get_error_code()
{
	return (rand() % 3000) + 1555;
}

int	dcg_proc_context_create(size_t initial_var_capacity, dst_func *function, dvm_context *context, dcg_proc_context *proc_context)
{
	proc_context->vars_named_count = 0;
	proc_context->vars_temp_count = 0;
	proc_context->vars_max_allocated = 0;

	proc_context->named_vars_capacity = initial_var_capacity == 0 ? 4 : initial_var_capacity;
	proc_context->named_vars = (dcg_var_binding *)malloc(sizeof(dcg_var_binding) * proc_context->named_vars_capacity);

	if (proc_context->named_vars == NULL)
	{
		return 0;
	}

	if (!dvm_procgen_begin_create(&proc_context->procgen, context))
	{
		return 0;
	}

	proc_context->function = function;
	proc_context->context = context;

	return 1;
}
void dcg_proc_context_destroy(dcg_proc_context *proc_context)
{
	if (proc_context->named_vars != NULL)
	{
		free(proc_context->named_vars);
	}
}

dcg_var_binding *dcg_proc_context_map(const char *name, dcg_proc_context *proc_context)
{
	uint32_t hashed_name = dsh_hash(name);

	for (size_t i = 0; i < proc_context->vars_named_count; ++i)
	{
		if (proc_context->named_vars[i].hashed_name == hashed_name)
		{
			return &proc_context->named_vars[i];
		}
	}

	return NULL;
}

size_t	dcg_proc_context_push_temp(dcg_proc_context *proc_context)
{
	if (proc_context->vars_named_count + proc_context->vars_temp_count >= 255)
		return ~0;

	size_t result = (proc_context->vars_temp_count++) + proc_context->vars_named_count;

	if (proc_context->vars_max_allocated < proc_context->vars_temp_count + proc_context->vars_named_count)
		proc_context->vars_max_allocated = proc_context->vars_temp_count + proc_context->vars_named_count;

	return result;
}
void	dcg_proc_context_pop_temp_to(size_t temp_reg_index, dcg_proc_context *proc_context)
{
	assert(temp_reg_index >= proc_context->vars_named_count);
	assert(temp_reg_index <= proc_context->vars_named_count + proc_context->vars_temp_count);

	proc_context->vars_temp_count = temp_reg_index - proc_context->vars_named_count + 1;
}
void	dcg_proc_context_pop_temp_past(size_t temp_reg_index, dcg_proc_context *proc_context)
{
	assert(temp_reg_index >= proc_context->vars_named_count);
	assert(temp_reg_index <= proc_context->vars_named_count + proc_context->vars_temp_count);

	proc_context->vars_temp_count = temp_reg_index - proc_context->vars_named_count;
}

size_t	dcg_proc_context_push_named(const char *name, dst_type type, dcg_proc_context *proc_context)
{
	if (proc_context->vars_named_count >= 255)
	{
		return ~0;
	}

	if (dcg_proc_context_map(name, proc_context) != NULL)
	{
		return ~0;
	}

	if (proc_context->vars_named_count == proc_context->named_vars_capacity)
	{
		proc_context->named_vars_capacity *= 2;

		dcg_var_binding *new_stack = (dcg_var_binding *)malloc(sizeof(dcg_var_binding) * proc_context->named_vars_capacity);
		
		if (new_stack == NULL)
		{
			return ~0;
		}

		memcpy(new_stack, proc_context->named_vars, sizeof(dcg_var_binding) * proc_context->vars_named_count);

		free(proc_context->named_vars);

		proc_context->named_vars = new_stack;
	}

	// For when we take a register from an expression that just evaluated

	if (proc_context->vars_temp_count > 0)
		proc_context->vars_temp_count--;

	proc_context->named_vars[proc_context->vars_named_count].hashed_name = dsh_hash(name);
	proc_context->named_vars[proc_context->vars_named_count].reg_index = proc_context->vars_named_count;
	proc_context->named_vars[proc_context->vars_named_count].type = type;

	return proc_context->vars_named_count++;
}
void	dcg_proc_context_pop_named_to(size_t named_reg_index, dcg_proc_context *proc_context)
{
	assert(named_reg_index <= proc_context->vars_named_count);
	assert(proc_context->vars_temp_count == 0);

	proc_context->vars_named_count = named_reg_index + 1;
}
void	dcg_proc_context_pop_named_past(size_t named_reg_index, dcg_proc_context *proc_context)
{
	assert(named_reg_index <= proc_context->vars_named_count);
	assert(proc_context->vars_temp_count == 0);

	proc_context->vars_named_count = named_reg_index;
}

int		dcg_proc_context_is_named(size_t reg_index, dcg_proc_context *proc_context)
{
	return reg_index < proc_context->vars_named_count;
}
int		dcg_proc_context_is_temp(size_t reg_index, dcg_proc_context *proc_context)
{
	return reg_index >= proc_context->vars_named_count;
}

dvm_bc  *dcg_proc_context_push_bc(size_t amount, dcg_proc_context *proc_context)
{
	return dvm_procgen_push_bc(amount, &proc_context->procgen);
}

int dcg_import_expression(
	dst_exp *exp,
	size_t *out_reg,
	dst_type_list **out_type,
	dcg_proc_context *proc_context
	)
{
	switch (exp->type)
	{
		case dst_exp_type_variable:
		{
			dcg_var_binding	*binding = dcg_proc_context_map(exp->variable.id, proc_context);

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
			size_t result_reg = dcg_proc_context_push_temp(proc_context);

			if (result_reg == ~0)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
				return 0;
			}

			dvm_bc *bc = dcg_proc_context_push_bc(2, proc_context);

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
				proc_context))
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

			if (dcg_proc_context_is_named(source_register, proc_context))
			{
				result_register = dcg_proc_context_push_temp(proc_context);

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

			dvm_bc *bc = dcg_proc_context_push_bc(1, proc_context);

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
					proc_context))
				{
					return 0;
				}
				if (!dcg_import_expression(
					exp->operator.right,
					&right_exp_register,
					&right_exp_type,
					proc_context))
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
					proc_context))
				{
					return 0;
				}
				if (!dcg_import_expression(
					exp->operator.left,
					&left_exp_register,
					&left_exp_type,
					proc_context))
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

			int left_is_named = dcg_proc_context_is_named(left_exp_register, proc_context);
			int right_is_named = dcg_proc_context_is_named(right_exp_register, proc_context);
			
			if (left_is_named && right_is_named)
			{
				result_register = dcg_proc_context_push_temp(proc_context);

				if (result_register == ~0)
				{
					fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
					return 0;
				}
			}
			else if (left_is_named)
			{
				result_register = right_exp_register;
				dcg_proc_context_pop_temp_to(right_exp_register, proc_context);
			}
			else if (right_is_named)
			{
				result_register = left_exp_register;
				dcg_proc_context_pop_temp_to(left_exp_register, proc_context);
			}
			else
			{
				// Use the lowest temporary register we can. This is will be the one for the expression executed first.

				if (exp->operator.left->temp_count_est >= exp->operator.right->temp_count_est)
				{
					result_register = left_exp_register;
					dcg_proc_context_pop_temp_to(left_exp_register, proc_context);
				}
				else
				{
					result_register = right_exp_register;
					dcg_proc_context_pop_temp_to(right_exp_register, proc_context);
				}
			}

			dvm_bc *bc = dcg_proc_context_push_bc(1, proc_context);

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

int dcg_import_statement(
	dst_statement *statement,
	dcg_proc_context *proc_context
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

				if (!dcg_import_expression(cur_exp->value, &exp_reg_start, &exp_types, proc_context))
				{
					return 0;
				}

				// Move the values into the variable registers
				// and define the variables in proc_context with the proper type

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid definition, cannot have an expression with values\n", get_error_code());
					return 0;
				}

				dst_type_list	*sub_val_type = exp_types;
				size_t			 sub_val_index = 0;

				while (1)
				{
					size_t val_reg = exp_reg_start + sub_val_index;
					size_t var_reg = dcg_proc_context_push_named(cur_var->value, sub_val_type->value, proc_context);

					if (var_reg == ~0)
					{
						fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
						return 0;
					}

					// Store the value in the variable, if it's not already there

					if (var_reg != val_reg)
					{
						dvm_bc *bc = dcg_proc_context_push_bc(1, proc_context);

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

					if (sub_val_type == exp_types)
						break;
				}

				// Go to the next expression for its values
				cur_exp = cur_exp->next;

				// Check to see if we ran out of vars before sub vals or expressions

				if (cur_var == statement->definition.variables && !(sub_val_type == exp_types || cur_exp == statement->definition.values))
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

				if (!dcg_import_expression(cur_exp->value, &exp_reg_start, &exp_types, proc_context))
				{
					return 0;
				}

				// Move the values into the variable registers
				// and define the variables in proc_context with the proper type

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
					dcg_var_binding *var = dcg_proc_context_map(cur_var->value, proc_context);

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
						dvm_bc *bc = dcg_proc_context_push_bc(1, proc_context);

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

					if (sub_val_type == exp_types)
						break;
				}

				// Clear out the temp registers we were using

				if (dcg_proc_context_is_temp(exp_reg_start, proc_context))
				{
					dcg_proc_context_pop_temp_past(exp_reg_start, proc_context);
				}

				// Go to the next expression for its values
				cur_exp = cur_exp->next;

				// Check to see if we ran out of vars before sub vals or expressions

				if (cur_var == statement->assignment.variables && !(sub_val_type == exp_types || cur_exp == statement->assignment.values))
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

			} while (cur_exp != statement->assignment.values);

			return 1;
		}

		fprintf(stderr, "error dsc%i: invalid assignment, zero ids or exps\n", get_error_code());
		return 0;
	}

	case dst_statement_type_call:
	{
		fprintf(stderr, "error dsc%i: calls are not implemented\n", get_error_code());

		return 0;

		/*size_t call_index = dvm_find_proction_index(dsh_hash(statement->call.function), proc_context->context);

		if (call_index == ~0)
		{
			return 0;
		}*/
	}

	case dst_statement_type_block:
	{
		size_t base_register = proc_context->vars_named_count;

		dst_statement_list *current = statement->block.statements;
		if (current != NULL)
		{
			do
			{
				if (!dcg_import_statement(current->value, proc_context))
				{
					return 0;
				}

				current = current->next;
			} while (current != statement->block.statements);
		}

		dcg_proc_context_pop_named_past(base_register, proc_context);

		return 1;
	}

	case dst_statement_type_if:
	{
		size_t			 cond_register;
		dst_type_list	*cond_type;

		// Write the if conditional first

		if (!dcg_import_expression(statement->if_else.condition, &cond_register, &cond_type, proc_context))
		{
			return 0;
		}

		if (!dst_type_list_is_integer(cond_type))
		{
			fprintf(stderr, "error dsc%i: invalid if statement, conditional must be an integer\n", get_error_code());
			return 0;
		}

		// We don't need to reserve this condition register past the first jmpc

		if (dcg_proc_context_is_temp(cond_register, proc_context))
		{
			dcg_proc_context_pop_temp_past(cond_register, proc_context);
		}

		// Write the jmp that will skip the false block and execute the true block

		size_t	jmp_to_true_loc = proc_context->procgen.bytecode_allocated;
		dvm_bc *jmp_to_true = dcg_proc_context_push_bc(1, proc_context);
		if (jmp_to_true == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_to_true[0].opcode = dvm_opcode_jmp_c;
		jmp_to_true[0].a = cond_register;

		// Write the false statement

		if (!dcg_import_statement(statement->if_else.false_statement, proc_context))
		{
			return 0;
		}

		// Write the jmp to skip the true statement after the false statement

		size_t	jmp_to_end_loc = proc_context->procgen.bytecode_allocated;
		dvm_bc *jmp_to_end = dcg_proc_context_push_bc(1, proc_context);
		if (jmp_to_end == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_to_end[0].opcode = dvm_opcode_jmp_u;

		// Write the true statement

		size_t true_statement_start_loc = proc_context->procgen.bytecode_allocated;
		if (!dcg_import_statement(statement->if_else.true_statement, proc_context))
		{
			return 0;
		}
		size_t true_statement_end_loc = proc_context->procgen.bytecode_allocated;

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

		size_t cond_loc = proc_context->procgen.bytecode_allocated;

		if (!dcg_import_expression(statement->while_loop.condition, &cond_register, &cond_type, proc_context))
		{
			return 0;
		}

		if (!dst_type_list_is_integer(cond_type))
		{
			fprintf(stderr, "error dsc%i: invalid while statement, conditional must be an integer\n", get_error_code());
			return 0;
		}

		// We don't need to reserve this condition register after the jmpc

		if (dcg_proc_context_is_temp(cond_register, proc_context))
		{
			dcg_proc_context_pop_temp_past(cond_register, proc_context);
		}

		// Write the jmp to skip the block

		size_t	jmp_break_loc = proc_context->procgen.bytecode_allocated;
		dvm_bc *jmp_break = dcg_proc_context_push_bc(1, proc_context);
		if (jmp_break == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_break[0].opcode = dvm_opcode_jmp_cn;
		jmp_break[0].a = cond_register;

		// Write the while body

		if (!dcg_import_statement(statement->while_loop.loop_statement, proc_context))
		{
			return 0;
		}

		// Write the jmp to go back to the conditional

		size_t	jmp_continue_loc = proc_context->procgen.bytecode_allocated;
		dvm_bc *jmp_continue = dcg_proc_context_push_bc(1, proc_context);
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
		dst_type_list *cur_out = proc_context->function->out_types;
		dst_exp_list *cur_exp = statement->ret.values;

		if (cur_out == NULL && cur_exp != NULL)
		{
			fprintf(stderr, "error dsc%i: invalid return statement, no out values expected in function\n", get_error_code());
			return 0;
		}

		size_t ldst_out_register = 0;
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

				if (!dcg_import_expression(cur_exp->value, &exp_reg_start, &exp_types, proc_context))
				{
					return 0;
				}

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid return statement, cannot have an expression with zero values\n", get_error_code());
					return 0;
				}

				// Clear out the temp registers we were using, this allows us to reclaim them with new temp variables for output

				if (dcg_proc_context_is_temp(exp_reg_start, proc_context))
				{
					dcg_proc_context_pop_temp_past(exp_reg_start, proc_context);
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
					size_t out_reg = dcg_proc_context_push_temp(proc_context);

					if (out_reg == ~0)
					{
						fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
						return 0;
					}

					ldst_out_register = out_reg;

					// Store the value in the out register, if it's not already there

					if (out_reg != sub_val_reg)
					{
						dvm_bc *bc = dcg_proc_context_push_bc(1, proc_context);

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

					if (cur_out == proc_context->function->out_types || sub_val_type == exp_types)
						break;
				}

				// Go to the next expression for its values
				cur_exp = cur_exp->next;

				// Check to see if we ran out of outs before sub vals or expressions

				if (cur_out == proc_context->function->out_types && !(sub_val_type == exp_types || cur_exp == statement->ret.values))
				{
					fprintf(stderr, "error dsc%i: invalid return statement, too many out values\n", get_error_code());
					return 0;
				}

				// Check to see if we ran out of expressions before outs

				if (cur_out != proc_context->function->out_types && cur_exp == statement->ret.values)
				{
					fprintf(stderr, "error dsc%i: invalid return statement, not enough out values\n", get_error_code());
					return 0;
				}

			} while (cur_exp != statement->ret.values);

			dvm_bc *ret_bc = dcg_proc_context_push_bc(1, proc_context);

			if (ret_bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}

			ret_bc[0].opcode = dvm_opcode_ret;
			ret_bc[0].a = ldst_out_register - (out_count - 1);

			dcg_proc_context_pop_temp_past(ldst_out_register - (out_count - 1), proc_context);

			return 1;
		}
		else
		{
			dvm_bc *ret_bc = dcg_proc_context_push_bc(1, proc_context);

			if (ret_bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}

			ret_bc[0].opcode = dvm_opcode_ret;

			dcg_proc_context_pop_temp_past(ldst_out_register - (out_count - 1), proc_context);

			return 1;
		}

	}

	}

	fprintf(stderr, "error dsc%i: internal error, invalid statement in ast\n", get_error_code());
	return 0;
}

int dcg_import_function_params(dst_func_param_list *params, dcg_proc_context *proc_context)
{
	if (params == NULL)
		return 1;

	dst_func_param_list *current = params;

	do
	{
		if (dcg_proc_context_push_named(current->value->id, current->value->type, proc_context) == ~0)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
			return 0;
		}

		current = current->next;

	} while (current != params);

	return 1;
}

int dcg_import_function(dst_func *func, dvm_context *context)
{	
	size_t param_count = dst_func_param_list_count(func->in_params);
	size_t out_count = dst_type_list_count(func->out_types);

	if (param_count > 255 || out_count > 255)
	{
		fprintf(stderr, "error dsc%i: invalid function, too many params or outs\n", get_error_code());
		return 0;
	}

	dcg_proc_context proc_context;
	if (!dcg_proc_context_create(16, func, context, &proc_context))
	{
		fprintf(stderr, "error dsc%i: internal error, cannot allocate internal memory\n", get_error_code());

		return 0;
	}

	if (!dcg_import_function_params(func->in_params, &proc_context))
	{
		dvm_procgen_cancel(&proc_context.procgen);
		dcg_proc_context_destroy(&proc_context);

		return 0;
	}

	if (!dcg_import_statement(func->statement, &proc_context))
	{
		dvm_procgen_cancel(&proc_context.procgen);
		dcg_proc_context_destroy(&proc_context);

		return 0;
	}

	size_t vals_used = proc_context.vars_max_allocated - param_count;

	if (vals_used > 255)
	{
		fprintf(stderr, "error dsc%i: internal error, allocated too many registers\n", get_error_code());

		dvm_procgen_cancel(&proc_context.procgen);
		dcg_proc_context_destroy(&proc_context);

		return 0;
	}

	if (dvm_procgen_finalize(func->id, (uint8_t)param_count, (uint8_t)vals_used, (uint8_t)out_count, &proc_context.procgen) == NULL)
	{
		return 0;
	}

	dcg_proc_context_destroy(&proc_context);

	return 1;
}

int dcg_import_function_list(dst_func_list *list, dvm_context *context)
{
	dst_func_list *current = list;

	while (current != NULL)
	{
		if (!dcg_import_function(current->value, context))
		{
			return 0;
		}

		current = current->next;

		if (current == list)
			break;
	}

	return 1;
}

#include "parser.h"
#include <stdio.h>

typedef void *yyscan_t;
int yylex_init(yyscan_t * ptr_yy_globals);
int yylex(union YYSTYPE *yyval_param, struct YYLTYPE *yylloc_param, yyscan_t yyscanner);
int yylex_destroy(yyscan_t yyscanner);
void yyset_in(FILE *in, yyscan_t yyscanner);

void test_tokenize(yyscan_t scanner)
{
	YYSTYPE yylval;
	YYLTYPE yylloc;

	int token = 0;

	printf("$begin ");

	while ((token = yylex(&yylval, &yylloc, scanner)) != 0)
	{
		switch (token)
		{
		case TOKEN_INTEGER:
			printf("int(%d)", yylval.integer);
			break;
		case TOKEN_REAL:
			printf("real(%f)", yylval.real);
			break;
		case TOKEN_IDENTIFIER:
			printf("id(%s)", yylval.string);
			break;
		case TOKEN_TYPE:
			printf("type(%d)", yylval.type);
			break;
		case TOKEN_DEF:
			printf("def");
			break;
		case TOKEN_ARROW:
			printf("->");
			break;
		case TOKEN_LET:
			printf("let");
			break;
		case TOKEN_WHILE:
			printf("while");
			break;
		case TOKEN_IF:
			printf("if");
			break;
		case TOKEN_ELSE:
			printf("else");
			break;
		case TOKEN_RETURN:
			printf("return");
			break;
		default:
			printf("%c", token);
			break;
		}

		printf(" ");
	}

	printf("$end");
}

int dvm_import_source(const char *source_file, struct dvm_context *context)
{
	// seed the error code generator
	srand((unsigned int)time(NULL));

	yyscan_t scanner;
	if (yylex_init(&scanner) != 0)
	{
		fprintf(stderr, "couldn't init lex.\n");
		return 0;
	}

	FILE *source = fopen(source_file, "rb");
	if (source == NULL)
	{
		yylex_destroy(scanner);

		fprintf(stderr, "couldn't open source file %s\n", source_file);
		return 0;
	}
	yyset_in(source, scanner);

	//yydebug = 1;
	dst_func_list *module = NULL;

	if (yyparse(&module, scanner) == 0)
	{
		dst_print_func_list(module);

		if (!dcg_import_function_list(module, context))
		{
			dst_destroy_func_list(module);

			fclose(source);
			yylex_destroy(scanner);

			return 0;
		}
		else
		{
			dst_destroy_func_list(module);

			fclose(source);
			yylex_destroy(scanner);

			return 1;
		}
	}

	fclose(source);
	yylex_destroy(scanner);
	
	return 0;
}