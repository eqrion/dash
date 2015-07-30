#include "../hash.h"
#include "../context.h"
#include "ast.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

int ast_type_list_is_integer(ast_type_list *list)
{
	return (list->value == ast_type_integer) && (list->next == list);
}
int ast_type_list_is_real(ast_type_list *list)
{
	return (list->value == ast_type_real) && (list->next == list);
}
int ast_type_list_is_composite(ast_type_list *list)
{
	return list->next != list;
}

struct dsh_var_binding
{
	uint32_t		hashed_name;
	size_t			reg_index;
	ast_type		type;
};
struct dsh_funcgen_context
{
	size_t					vars_named_count;
	size_t					vars_temp_count;
	size_t					vars_max_allocated;

	size_t					named_vars_capacity;
	struct dsh_var_binding *named_vars;

	size_t bytecode_start;
	size_t bytecode_allocated;

	ast_func			*function;
	dsh_context			*context;
};
typedef struct dsh_var_binding dsh_var_binding;
typedef struct dsh_funcgen_context dsh_funcgen_context;

int get_error_code()
{
	return (rand() % 3000) + 1000;
}

int	dsh_funcgen_context_create(size_t initial_var_capacity, ast_func *function, dsh_context *context, dsh_funcgen_context *funcgen)
{
	funcgen->vars_named_count = 0;
	funcgen->vars_temp_count = 0;
	funcgen->vars_max_allocated = 0;

	funcgen->named_vars_capacity = initial_var_capacity == 0 ? 4 : initial_var_capacity;
	funcgen->named_vars = (dsh_var_binding *)malloc(sizeof(dsh_var_binding) * funcgen->named_vars_capacity);

	if (funcgen->named_vars == NULL)
		return 0;

	funcgen->bytecode_start = context->bytecode_count;
	funcgen->bytecode_allocated = 0;

	funcgen->function = function;
	funcgen->context = context;

	return 1;
}
void dsh_funcgen_context_destroy(dsh_funcgen_context *funcgen)
{

}

size_t	dsh_funcgen_context_push_temp(dsh_funcgen_context *funcgen)
{
	if (funcgen->vars_named_count + funcgen->vars_temp_count >= 255)
		return ~0;

	size_t result = (funcgen->vars_temp_count++) + funcgen->vars_named_count;

	if (funcgen->vars_max_allocated < funcgen->vars_temp_count + funcgen->vars_named_count)
		funcgen->vars_max_allocated = funcgen->vars_temp_count + funcgen->vars_named_count;

	return result;
}
void	dsh_funcgen_context_pop_temp_to(size_t temp_reg_index, dsh_funcgen_context *funcgen)
{
	assert(temp_reg_index >= funcgen->vars_named_count);
	assert(temp_reg_index <= funcgen->vars_named_count + funcgen->vars_temp_count);

	funcgen->vars_temp_count = temp_reg_index - funcgen->vars_named_count + 1;
}
void	dsh_funcgen_context_pop_temp_past(size_t temp_reg_index, dsh_funcgen_context *funcgen)
{
	assert(temp_reg_index >= funcgen->vars_named_count);
	assert(temp_reg_index <= funcgen->vars_named_count + funcgen->vars_temp_count);

	funcgen->vars_temp_count = temp_reg_index - funcgen->vars_named_count;
}

size_t	dsh_funcgen_context_push_named(const char *name, ast_type type, dsh_funcgen_context *funcgen)
{
	if (funcgen->vars_named_count >= 255)
	{
		return ~0;
	}

	if (funcgen->vars_named_count == funcgen->named_vars_capacity)
	{
		funcgen->named_vars_capacity *= 2;

		dsh_var_binding *new_stack = (dsh_var_binding *)malloc(sizeof(dsh_var_binding) * funcgen->named_vars_capacity);
		
		if (new_stack == NULL)
		{
			return ~0;
		}

		memcpy(new_stack, funcgen->named_vars, sizeof(dsh_var_binding) * funcgen->vars_named_count);

		free(funcgen->named_vars);

		funcgen->named_vars = new_stack;
	}

	// For when we take a register from an expression that just evaluated

	if (funcgen->vars_temp_count > 0)
		funcgen->vars_temp_count--;

	funcgen->named_vars[funcgen->vars_named_count].hashed_name = dsh_hash(name);
	funcgen->named_vars[funcgen->vars_named_count].reg_index = funcgen->vars_named_count;
	funcgen->named_vars[funcgen->vars_named_count].type = type;

	return funcgen->vars_named_count++;
}
void	dsh_funcgen_context_pop_named_to(size_t named_reg_index, dsh_funcgen_context *funcgen)
{
	assert(named_reg_index <= funcgen->vars_named_count);
	assert(funcgen->vars_temp_count == 0);

	funcgen->vars_named_count = named_reg_index + 1;
}
void	dsh_funcgen_context_pop_named_past(size_t named_reg_index, dsh_funcgen_context *funcgen)
{
	assert(named_reg_index <= funcgen->vars_named_count);
	assert(funcgen->vars_temp_count == 0);

	funcgen->vars_named_count = named_reg_index;
}

int		dsh_funcgen_context_is_named(size_t reg_index, dsh_funcgen_context *funcgen)
{
	return reg_index < funcgen->vars_named_count;
}
int		dsh_funcgen_context_is_temp(size_t reg_index, dsh_funcgen_context *funcgen)
{
	return reg_index >= funcgen->vars_named_count;
}

dsh_var_binding *dsh_funcgen_context_map(const char *name, dsh_funcgen_context *funcgen)
{
	uint32_t hashed_name = dsh_hash(name);

	for (size_t i = 0; i < funcgen->vars_named_count; ++i)
	{
		if (funcgen->named_vars[i].hashed_name == hashed_name)
		{
			return &funcgen->named_vars[i];
		}
	}

	return NULL;
}

dsh_bc  *dsh_funcgen_context_push_bc(size_t amount, dsh_funcgen_context *funcgen)
{
	funcgen->bytecode_allocated += amount;
	return dsh_context_push_bytecode(amount, funcgen->context);
}

int dsh_import_expression(
	ast_exp *exp,
	size_t *out_reg,
	ast_type_list **out_type,
	dsh_funcgen_context *funcgen
	)
{
	switch (exp->type)
	{
		case ast_exp_type_variable:
		{
			dsh_var_binding	*binding = dsh_funcgen_context_map(exp->variable.id, funcgen);

			if (binding == NULL)
			{
				fprintf(stderr, "error dsc%i: invalid variable identifier (%s)\n", get_error_code(), exp->variable.id);
				return 0;
			}

			(*out_reg) = binding->reg_index;
			(*out_type) = binding->type == ast_type_real ? &ast_sentinel_type_real : &ast_sentinel_type_integer;

			return 1;
		}
		break;

		case ast_exp_type_integer:
		case ast_exp_type_real:
		{
			size_t result_reg = dsh_funcgen_context_push_temp(funcgen);

			if (result_reg == ~0)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
				return 0;
			}

			dsh_bc *bc = dsh_funcgen_context_push_bc(2, funcgen);

			if (bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}

			bc[0].opcode = dsh_opcode_stor;
			bc[0].c = result_reg;

			if (exp->type == ast_type_integer)
			{
				*(int32_t *)(bc + 1) = exp->integer.value;
				(*out_type) = &ast_sentinel_type_integer;
			}
			else
			{
				*(float *)(bc + 1) = exp->real.value;
				(*out_type) = &ast_sentinel_type_real;
			}

			(*out_reg) = result_reg;

			return 1;
		}
		break;
				
		case ast_exp_type_cast:
		{
			size_t			 source_register;
			ast_type_list	*source_type;

			size_t	result_register;

			if (!dsh_import_expression(
				exp->cast.value,
				&source_register,
				&source_type,
				funcgen))
			{
				return 0;
			}

			if (ast_type_list_is_composite(source_type) ||
				ast_type_list_is_integer(source_type) && exp->cast.dest_type == ast_type_integer ||
				ast_type_list_is_real(source_type) && exp->cast.dest_type == ast_type_real)
			{
				fprintf(stderr, "error dsc%i: invalid cast expression\n", get_error_code());
				return 0;
			}

			if (dsh_funcgen_context_is_named(source_register, funcgen))
			{
				result_register = dsh_funcgen_context_push_temp(funcgen);

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

			dsh_bc *bc = dsh_funcgen_context_push_bc(1, funcgen);

			if (bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot bytecode \n", get_error_code());
				return 0;
			}

			if (exp->cast.dest_type == ast_type_real)
			{
				bc[0].opcode = dsh_opcode_castf;
				(*out_type) = &ast_sentinel_type_real;
			}
			else
			{
				bc[0].opcode = dsh_opcode_casti;
				(*out_type) = &ast_sentinel_type_integer;
			}

			bc[0].a = source_register;
			bc[0].c = result_register;
			
			(*out_reg) = result_register;

			return 1;
		}
		break;

		case ast_exp_type_addition:
		case ast_exp_type_subtraction:
		case ast_exp_type_multiplication:
		case ast_exp_type_division:
		case ast_exp_type_less:
		case ast_exp_type_less_eq:
		case ast_exp_type_greater:
		case ast_exp_type_greater_eq:
		{
			size_t			 left_exp_register;
			ast_type_list	*left_exp_type;
			size_t			 right_exp_register;
			ast_type_list	*right_exp_type;

			size_t result_register;

			if (exp->operator.left->temp_count_est >= exp->operator.right->temp_count_est)
			{
				if (!dsh_import_expression(
					exp->operator.left,
					&left_exp_register,
					&left_exp_type,
					funcgen))
				{
					return 0;
				}
				if (!dsh_import_expression(
					exp->operator.right,
					&right_exp_register,
					&right_exp_type,
					funcgen))
				{
					return 0;
				}
			}
			else
			{
				if (!dsh_import_expression(
					exp->operator.right,
					&right_exp_register,
					&right_exp_type,
					funcgen))
				{
					return 0;
				}
				if (!dsh_import_expression(
					exp->operator.left,
					&left_exp_register,
					&left_exp_type,
					funcgen))
				{
					return 0;
				}
			}

			if (ast_type_list_is_composite(left_exp_type) || ast_type_list_is_composite(right_exp_type))
			{
				fprintf(stderr, "error dsc%i: invalid operands to binary expression, cannot be composite types\n", get_error_code());
				return 0;
			}

			if (left_exp_type->value != right_exp_type->value)
			{
				fprintf(stderr, "error dsc%i: invalid operands to binary expression, mismatch integer, real\n", get_error_code());
				return 0;
			}

			int left_is_named = dsh_funcgen_context_is_named(left_exp_register, funcgen);
			int right_is_named = dsh_funcgen_context_is_named(right_exp_register, funcgen);
			
			if (left_is_named && right_is_named)
			{
				result_register = dsh_funcgen_context_push_temp(funcgen);

				if (result_register == ~0)
				{
					fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
					return 0;
				}
			}
			else if (left_is_named)
			{
				result_register = right_exp_register;
				dsh_funcgen_context_pop_temp_to(right_exp_register, funcgen);
			}
			else if (right_is_named)
			{
				result_register = left_exp_register;
				dsh_funcgen_context_pop_temp_to(left_exp_register, funcgen);
			}
			else
			{
				// Use the lowest temporary register we can. This is will be the one for the expression executed first.

				if (exp->operator.left->temp_count_est >= exp->operator.right->temp_count_est)
				{
					result_register = left_exp_register;
					dsh_funcgen_context_pop_temp_to(left_exp_register, funcgen);
				}
				else
				{
					result_register = right_exp_register;
					dsh_funcgen_context_pop_temp_to(right_exp_register, funcgen);
				}
			}

			dsh_bc *bc = dsh_funcgen_context_push_bc(1, funcgen);

			if (bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}
 
			switch (exp->type)
			{
			case ast_exp_type_addition:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_addi : dsh_opcode_addf;
				bc[0].a = left_exp_register;
				bc[0].b = right_exp_register;
				bc[0].c = result_register;

				(*out_type) = left_exp_type;
				break;
			case ast_exp_type_subtraction:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_subi : dsh_opcode_subf;
				bc[0].a = left_exp_register;
				bc[0].b = right_exp_register;
				bc[0].c = result_register;

				(*out_type) = left_exp_type;
				break;
			case ast_exp_type_multiplication:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_muli : dsh_opcode_mulf;
				bc[0].a = left_exp_register;
				bc[0].b = right_exp_register;
				bc[0].c = result_register;

				(*out_type) = left_exp_type;
				break;
			case ast_exp_type_division:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_divi : dsh_opcode_divf;
				bc[0].a = left_exp_register;
				bc[0].b = right_exp_register;
				bc[0].c = result_register;

				(*out_type) = left_exp_type;
				break;

			case ast_exp_type_less:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_cmpi_l : dsh_opcode_cmpf_l;
				bc[0].a = left_exp_register;
				bc[0].b = right_exp_register;
				bc[0].c = result_register;

				(*out_type) = &ast_sentinel_type_integer;
				break;
			case ast_exp_type_less_eq:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_cmpi_le : dsh_opcode_cmpf_le;
				bc[0].a = left_exp_register;
				bc[0].b = right_exp_register;
				bc[0].c = result_register;

				(*out_type) = &ast_sentinel_type_integer;
				break;
			case ast_exp_type_greater:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_cmpi_l : dsh_opcode_cmpf_l;
				bc[0].a = right_exp_register;
				bc[0].b = left_exp_register;
				bc[0].c = result_register;

				(*out_type) = &ast_sentinel_type_integer;
				break;
			case ast_exp_type_greater_eq:
				bc[0].opcode = left_exp_type->value == ast_type_integer ? dsh_opcode_cmpi_le : dsh_opcode_cmpf_le;
				bc[0].a = right_exp_register;
				bc[0].b = left_exp_register;
				bc[0].c = result_register;

				(*out_type) = &ast_sentinel_type_integer;
			break;
			}

			(*out_reg) = result_register;

			return 1;
		}
		break;

		case ast_exp_type_call:
		{
			fprintf(stderr, "error dsc%i: calls are not implemented\n", get_error_code());
			return 0;
		}
		break;
	}

	fprintf(stderr, "error dsc%i: internal error, invalid expression in ast\n", get_error_code());
	return 0;
}

int dsh_import_statement(
	ast_statement *statement,
	dsh_funcgen_context *funcgen
	)
{
	switch (statement->type)
	{

	case ast_statement_type_definition:
	{
		ast_id_list *cur_var = statement->definition.variables;
		ast_exp_list *cur_exp = statement->definition.values;

		if (cur_var != NULL && cur_exp != NULL)
		{
			do
			{
				size_t			 exp_reg_start;
				ast_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dsh_import_expression(cur_exp->value, &exp_reg_start, &exp_types, funcgen))
				{
					return 0;
				}

				// Move the values into the variable registers
				// and define the variables in funcgen with the proper type

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid definition, cannot have an expression with values\n", get_error_code());
					return 0;
				}

				ast_type_list	*sub_val_type = exp_types;
				size_t			 sub_val_index = 0;

				while (1)
				{
					size_t val_reg = exp_reg_start + sub_val_index;
					size_t var_reg = dsh_funcgen_context_push_named(cur_var->value, sub_val_type->value, funcgen);

					if (var_reg == ~0)
					{
						fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
						return 0;
					}

					// Store the value in the variable, if it's not already there

					if (var_reg != val_reg)
					{
						dsh_bc *bc = dsh_funcgen_context_push_bc(1, funcgen);

						if (bc == NULL)
						{
							fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
							return 0;
						}

						bc[0].opcode = dsh_opcode_mov;
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

	case ast_statement_type_assignment:
	{
		ast_id_list *cur_var = statement->assignment.variables;
		ast_exp_list *cur_exp = statement->assignment.values;

		if (cur_var != NULL && cur_exp != NULL)
		{
			do
			{
				size_t			 exp_reg_start;
				ast_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dsh_import_expression(cur_exp->value, &exp_reg_start, &exp_types, funcgen))
				{
					return 0;
				}

				// Move the values into the variable registers
				// and define the variables in funcgen with the proper type

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid assignment, cannot have an expression with zero values\n", get_error_code());
					return 0;
				}

				ast_type_list *sub_val_type = exp_types;
				size_t sub_val_index = 0;

				while (1)
				{
					size_t val_reg = exp_reg_start + sub_val_index;
					dsh_var_binding *var = dsh_funcgen_context_map(cur_var->value, funcgen);

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
						dsh_bc *bc = dsh_funcgen_context_push_bc(1, funcgen);

						if (bc == NULL)
						{
							fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
							return 0;
						}

						bc[0].opcode = dsh_opcode_mov;
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

				if (dsh_funcgen_context_is_temp(exp_reg_start, funcgen))
				{
					dsh_funcgen_context_pop_temp_past(exp_reg_start, funcgen);
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

	case ast_statement_type_call:
	{
		fprintf(stderr, "error dsc%i: calls are not implemented\n", get_error_code());

		return 0;

		/*size_t call_index = dsh_context_find_function_index(dsh_hash(statement->call.function), funcgen->context);

		if (call_index == ~0)
		{
			return 0;
		}*/
	}

	case ast_statement_type_block:
	{
		size_t base_register = funcgen->vars_named_count;

		ast_statement_list *current = statement->block.statements;
		if (current != NULL)
		{
			do
			{
				if (!dsh_import_statement(current->value, funcgen))
				{
					return 0;
				}

				current = current->next;
			} while (current != statement->block.statements);
		}

		dsh_funcgen_context_pop_named_past(base_register, funcgen);

		return 1;
	}

	case ast_statement_type_if:
	{
		size_t			 cond_register;
		ast_type_list	*cond_type;

		// Write the if conditional first

		if (!dsh_import_expression(statement->if_else.condition, &cond_register, &cond_type, funcgen))
		{
			return 0;
		}

		if (!ast_type_list_is_integer(cond_type))
		{
			fprintf(stderr, "error dsc%i: invalid if statement, conditional must be an integer\n", get_error_code());
			return 0;
		}

		// We don't need to reserve this condition register past the first jmpc

		if (dsh_funcgen_context_is_temp(cond_register, funcgen))
		{
			dsh_funcgen_context_pop_temp_past(cond_register, funcgen);
		}

		// Write the jmp that will skip the false block and execute the true block

		size_t	jmp_to_true_loc = funcgen->bytecode_allocated;
		dsh_bc *jmp_to_true = dsh_funcgen_context_push_bc(1, funcgen);
		if (jmp_to_true == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_to_true[0].opcode = dsh_opcode_jmp_c;
		jmp_to_true[0].a = cond_register;

		// Write the false statement

		if (!dsh_import_statement(statement->if_else.false_statement, funcgen))
		{
			return 0;
		}

		// Write the jmp to skip the true statement after the false statement

		size_t	jmp_to_end_loc = funcgen->bytecode_allocated;
		dsh_bc *jmp_to_end = dsh_funcgen_context_push_bc(1, funcgen);
		if (jmp_to_end == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_to_end[0].opcode = dsh_opcode_jmp_u;

		// Write the true statement

		size_t true_statement_start_loc = funcgen->bytecode_allocated;
		if (!dsh_import_statement(statement->if_else.true_statement, funcgen))
		{
			return 0;
		}
		size_t true_statement_end_loc = funcgen->bytecode_allocated;

		// Resolve jmp offsets

		int8_t offset1 = (int8_t)(((int)true_statement_start_loc) - ((int)jmp_to_true_loc));
		jmp_to_true[0].c = *(uint8_t *)&offset1;

		int8_t offset2 = (int8_t)((int)true_statement_end_loc - (int)jmp_to_end_loc);
		jmp_to_end[0].c = *(uint8_t *)&offset2;

		return 1;
	}

	case ast_statement_type_while:
	{
		size_t			 cond_register;
		ast_type_list	*cond_type;

		// Write the expression for the while condition

		size_t cond_loc = funcgen->bytecode_allocated;

		if (!dsh_import_expression(statement->while_loop.condition, &cond_register, &cond_type, funcgen))
		{
			return 0;
		}

		if (!ast_type_list_is_integer(cond_type))
		{
			fprintf(stderr, "error dsc%i: invalid while statement, conditional must be an integer\n", get_error_code());
			return 0;
		}

		// We don't need to reserve this condition register after the jmpc

		if (dsh_funcgen_context_is_temp(cond_register, funcgen))
		{
			dsh_funcgen_context_pop_temp_past(cond_register, funcgen);
		}

		// Write the jmp to skip the block

		size_t	jmp_break_loc = funcgen->bytecode_allocated;
		dsh_bc *jmp_break = dsh_funcgen_context_push_bc(1, funcgen);
		if (jmp_break == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_break[0].opcode = dsh_opcode_jmp_cn;
		jmp_break[0].a = cond_register;

		// Write the while body

		if (!dsh_import_statement(statement->while_loop.loop_statement, funcgen))
		{
			return 0;
		}

		// Write the jmp to go back to the conditional

		size_t	jmp_continue_loc = funcgen->bytecode_allocated;
		dsh_bc *jmp_continue = dsh_funcgen_context_push_bc(1, funcgen);
		if (jmp_continue == NULL)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
			return 0;
		}
		jmp_continue[0].opcode = dsh_opcode_jmp_u;

		// Resolve the jmp offsets

		int8_t offset1 = (int8_t)((int)cond_loc - (int)jmp_continue_loc);
		jmp_continue[0].c = *(uint8_t *)&offset1;

		int8_t offset2 = (int8_t)((int)jmp_continue_loc + 1 - (int)jmp_break_loc);
		jmp_break[0].c = *(uint8_t *)&offset2;

		return 1;
	}

	case ast_statement_type_return:
	{
		ast_type_list *cur_out = funcgen->function->out_types;
		ast_exp_list *cur_exp = statement->ret.values;

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
				ast_type_list	*exp_types;

				// Evaluate the expression, returning n >= 1 possible values
				// n is the length of the exp_types linked list
				// the register storing n is exp_reg_start + n

				if (!dsh_import_expression(cur_exp->value, &exp_reg_start, &exp_types, funcgen))
				{
					return 0;
				}

				if (exp_types == NULL)
				{
					fprintf(stderr, "error dsc%i: invalid return statement, cannot have an expression with zero values\n", get_error_code());
					return 0;
				}

				// Clear out the temp registers we were using, this allows us to reclaim them with new temp variables for output

				if (dsh_funcgen_context_is_temp(exp_reg_start, funcgen))
				{
					dsh_funcgen_context_pop_temp_past(exp_reg_start, funcgen);
				}

				// Move the values into the output registers

				ast_type_list	*sub_val_type = exp_types;
				size_t			 sub_val_index = 0;

				while (1)
				{
					if (sub_val_type->value != cur_out->value)
					{
						fprintf(stderr, "error dsc%i: invalid return statement, out type mismatch\n", get_error_code());
						return 0;
					}

					size_t sub_val_reg = exp_reg_start + sub_val_index;
					size_t out_reg = dsh_funcgen_context_push_temp(funcgen);

					if (out_reg == ~0)
					{
						fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
						return 0;
					}

					last_out_register = out_reg;

					// Store the value in the out register, if it's not already there

					if (out_reg != sub_val_reg)
					{
						dsh_bc *bc = dsh_funcgen_context_push_bc(1, funcgen);

						if (bc == NULL)
						{
							fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
							return 0;
						}

						bc[0].opcode = dsh_opcode_mov;
						bc[0].a = sub_val_reg;
						bc[0].c = out_reg;
					}

					// Go to the next value of this expression

					++sub_val_index;
					sub_val_type = sub_val_type->next;

					// Go to the next out of this ret

					cur_out = cur_out->next;
					++out_count;

					if (cur_out == funcgen->function->out_types || sub_val_type == exp_types)
						break;
				}

				// Go to the next expression for its values
				cur_exp = cur_exp->next;

				// Check to see if we ran out of outs before sub vals or expressions

				if (cur_out == funcgen->function->out_types && !(sub_val_type == exp_types || cur_exp == statement->ret.values))
				{
					fprintf(stderr, "error dsc%i: invalid return statement, too many out values\n", get_error_code());
					return 0;
				}

				// Check to see if we ran out of expressions before outs

				if (cur_out != funcgen->function->out_types && cur_exp == statement->ret.values)
				{
					fprintf(stderr, "error dsc%i: invalid return statement, not enough out values\n", get_error_code());
					return 0;
				}

			} while (cur_exp != statement->ret.values);

			dsh_bc *ret_bc = dsh_funcgen_context_push_bc(1, funcgen);

			if (ret_bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}

			ret_bc[0].opcode = dsh_opcode_ret;
			ret_bc[0].a = last_out_register - (out_count - 1);

			dsh_funcgen_context_pop_temp_past(last_out_register - (out_count - 1), funcgen);

			return 1;
		}
		else
		{
			dsh_bc *ret_bc = dsh_funcgen_context_push_bc(1, funcgen);

			if (ret_bc == NULL)
			{
				fprintf(stderr, "error dsc%i: internal error, cannot allocate bytecode\n", get_error_code());
				return 0;
			}

			ret_bc[0].opcode = dsh_opcode_ret;

			dsh_funcgen_context_pop_temp_past(last_out_register - (out_count - 1), funcgen);

			return 1;
		}

	}

	}

	fprintf(stderr, "error dsc%i: internal error, invalid statement in ast\n", get_error_code());
	return 0;
}

int dsh_import_function_params(ast_func_param_list *params, dsh_funcgen_context *funcgen)
{
	if (params == NULL)
		return 1;

	ast_func_param_list *current = params;

	do
	{
		if (dsh_funcgen_context_push_named(current->value->id, current->value->type, funcgen) == ~0)
		{
			fprintf(stderr, "error dsc%i: internal error, cannot allocate register\n", get_error_code());
			return 0;
		}

		current = current->next;

	} while (current != params);

	return 1;
}

int dsh_import_function(ast_func *func, dsh_context *context)
{
	dsh_function_def *def = dsh_context_push_function(1, context);
	
	if (def == NULL)
	{
		fprintf(stderr, "error dsc%i: internal error, cannot allocate function\n", get_error_code());
		return 0;
	}
	
	size_t param_count = ast_func_param_list_count(func->in_params);
	size_t out_count = ast_type_list_count(func->out_types);

	if (param_count > 255 || out_count > 255)
	{
		fprintf(stderr, "error dsc%i: invalid function, too many params or outs\n", get_error_code());
		return 0;
	}

	def->hashed_name = dsh_hash(func->id);
	def->reg_count_in = (uint8_t)param_count;
	def->reg_count_out = (uint8_t)out_count;

	dsh_funcgen_context funcgen;

	if (!dsh_funcgen_context_create(16, func, context, &funcgen))
	{
		fprintf(stderr, "error dsc%i: internal error, cannot allocate internal memory\n", get_error_code());
		dsh_context_pop_function(1, context);

		return 0;
	}

	if (!dsh_import_function_params(func->in_params, &funcgen))
	{
		dsh_context_pop_bytecode(funcgen.bytecode_allocated, context);
		dsh_context_pop_function(1, context);

		dsh_funcgen_context_destroy(&funcgen);

		return 0;
	}

	if (!dsh_import_statement(func->statement, &funcgen))
	{
		dsh_context_pop_bytecode(funcgen.bytecode_allocated, context);
		dsh_context_pop_function(1, context);

		dsh_funcgen_context_destroy(&funcgen);

		return 0;
	}

	size_t vals_used = funcgen.vars_max_allocated - def->reg_count_in;

	if (vals_used > 255)
	{
		fprintf(stderr, "error dsc%i: internal error, allocated too many registers\n", get_error_code());

		dsh_context_pop_bytecode(funcgen.bytecode_allocated, context);
		dsh_context_pop_function(1, context);

		dsh_funcgen_context_destroy(&funcgen);

		return 0;
	}

	def->reg_count_use = (uint8_t)vals_used;

	def->bytecode_start = funcgen.bytecode_start;
	def->bytecode_end = funcgen.bytecode_start + funcgen.bytecode_allocated;

	dsh_funcgen_context_destroy(&funcgen);

	return 1;
}

int dsh_import_function_list(ast_func_list *list, dsh_context *context)
{
	ast_func_list *current = list;

	while (current != NULL)
	{
		if (!dsh_import_function(current->value, context))
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

int dsh_context_import_source(const char *source_file, struct dsh_context *context)
{
	// seed the error code generator
	srand(time(NULL));

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
	ast_func_list *module = NULL;

	if (yyparse(&module, scanner) == 0)
	{
		ast_print_func_list(module);

		if (!dsh_import_function_list(module, context))
		{
			ast_destroy_func_list(module);

			fclose(source);
			yylex_destroy(scanner);

			return 0;
		}
		else
		{
			ast_destroy_func_list(module);

			fclose(source);
			yylex_destroy(scanner);

			return 1;
		}
	}

	fclose(source);
	yylex_destroy(scanner);
	
	return 0;
}