#include "memory.h"

#include <stdint.h>
#include <string.h>

void *dsc_alloc(size_t size, dsc_memory *mem)
{
	uintptr_t base = ((uintptr_t)mem->stack_top + 3) & ~3;
	uintptr_t new_top = base + size;

	if (new_top > (uintptr_t)mem->stack_end)
	{
		return NULL;
	}

	mem->stack_top = (char *)new_top;
	return (char *)base;
}
void dsc_clear(dsc_memory *mem)
{
	mem->stack_top = mem->stack_begin;
}

char *dsc_strdup(const char *string, dsc_memory *mem)
{
	size_t len = strlen(string);
	char *newbuf = dsc_alloc(len + 1, mem);
	if (newbuf == NULL)
		return NULL;
	memcpy(newbuf, string, len);
	newbuf[len] = 0;
	return newbuf;
}

int dsc_create(size_t size, dsc_memory *mem)
{
	if (size == 0)
		size = 1024 * 8;

	mem->stack_begin = malloc(size);

	if (!mem->stack_begin)
	{
		return 0;
	}

	mem->stack_top = mem->stack_begin;
	mem->stack_end = mem->stack_begin + size;

	return 1;
}
void dsc_destroy(dsc_memory *mem)
{
	if (mem->stack_begin)
	{
		free(mem->stack_begin);
		mem->stack_begin = NULL;
	}
}