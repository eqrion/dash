#ifndef dash_stack_h
#define dash_stack_h

#include <stdlib.h>
#include <stdint.h>

#include "dash/var.h"

/*
 * A register stack for use in dash execution.
 *
 * reg_top corresponds with the beginning of the memory allocation.
 * reg_bottom = reg_top + size, forming the valid addresses of [reg_top, reg_bottom)
 * reg_current is the top of the dash stack. Addresses allocated for dash are in the
 *		range of [reg_current, reg_bottom). reg_current is initialized to reg_bottom,
 *		meaning that the stack initially has nothing allocated. The stack grows from
 *		reg_bottom to reg_top, or from higher addresses to lower addresses.
 */
struct dvm_stack
{
	dvm_var *reg_top;
	dvm_var *reg_current;
	dvm_var *reg_bottom;
};

void dvm_stack_dealloc(struct dvm_stack *stack)
{
	if (stack->reg_top != NULL)
	{
		free(stack->reg_top);

		stack->reg_top = NULL;
		stack->reg_current = NULL;
		stack->reg_bottom = NULL;
	}
}
int dvm_stack_alloc(struct dvm_stack *stack, size_t size)
{
	dvm_stack_dealloc(stack);

	stack->reg_top = (dvm_var *)malloc(size * sizeof(dvm_var));

	if (stack->reg_top == NULL)
	{
		return 0;
	}

	stack->reg_bottom = stack->reg_top + size;
	stack->reg_current = stack->reg_bottom;

	return 1;
}
int dvm_stack_push(struct dvm_stack *stack, size_t amount)
{
	return (stack->reg_current = stack->reg_current - amount) > stack->reg_top;
}
int dvm_stack_pop(struct dvm_stack *stack, size_t amount)
{
	return (stack->reg_current = stack->reg_current + amount) <= stack->reg_bottom;
}

#endif
