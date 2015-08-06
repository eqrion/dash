#ifndef dash_compiler_common_h
#define dash_compiler_common_h

#include "ast.h"
#include "memory.h"

struct dsc_parse_context
{
	dsc_memory		 *memory;
	dst_proc_list	 *parsed_module;
};
typedef struct dsc_parse_context dsc_parse_context;

/* ;) */
#define dsc_error_code (rand() % 2000) + 2555

#define dsc_error_oor() fprintf(stderr, "error dsc%i: cannot allocate a register.\n", dsc_error_code)
#define dsc_error_oom() fprintf(stderr, "error dsc%i: out of memory.\n", dsc_error_code)
#define dsc_error_internal() fprintf(stderr, "error dsc%i: internal error.\n", dsc_error_code)
#define dsc_error(message, ...) fprintf(stderr, "error dsc%i: ", dsc_error_code); fprintf(stderr, message, __VA_ARGS__); fprintf(stderr, "\n");

#endif