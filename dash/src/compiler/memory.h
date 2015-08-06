#ifndef dash_compiler_memory_h
#define dash_compiler_memory_h

#include <stdlib.h>

struct dsc_memory
{
	char *stack_begin;
	char *stack_top;
	char *stack_end;
};
typedef struct dsc_memory dsc_memory;

void *dsc_alloc(size_t size, dsc_memory *mem);
void dsc_clear(dsc_memory *mem);

char *dsc_strdup(const char *string, dsc_memory *mem);

int dsc_create(size_t size, dsc_memory *mem);
void dsc_destroy(dsc_memory *mem);

#endif