#ifndef dash_stack_h
#define dash_stack_h

#include <stdlib.h>
#include <stdint.h>

typedef struct
{
	union
	{
		int32_t i;
		float f;
		uint32_t u;
	};
} dsh_var;

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
struct dsh_stack
{
	dsh_var *reg_top;
	dsh_var *reg_current;
	dsh_var *reg_bottom;
};

void dsh_stack_dealloc(struct dsh_stack *stack)
{
	if (stack->reg_top != NULL)
	{
		free(stack->reg_top);

		stack->reg_top = NULL;
		stack->reg_current = NULL;
		stack->reg_bottom = NULL;
	}
}
int dsh_stack_alloc(struct dsh_stack *stack, size_t size)
{
	dsh_stack_dealloc(stack);

	stack->reg_top = (dsh_var *)malloc(size * sizeof(dsh_var));

	if (stack->reg_top == NULL)
	{
		return 0;
	}

	stack->reg_bottom = stack->reg_top + size;
	stack->reg_current = stack->reg_bottom;

	return 1;
}
int dsh_stack_push(struct dsh_stack *stack, size_t amount)
{
	return (stack->reg_current = stack->reg_current - amount) > stack->reg_top;
}
int dsh_stack_pop(struct dsh_stack *stack, size_t amount)
{
	return (stack->reg_current = stack->reg_current + amount) <= stack->reg_bottom;
}

#endif
