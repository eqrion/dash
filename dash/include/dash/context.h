#ifndef dash_vm_h
#define dash_vm_h

#include <stdint.h>

#include "var.h"

struct dsh_context;

int dsh_create_context(struct dsh_context **context, size_t initial_function_capacity, size_t initial_bytecode_capacity);
void dsh_destroy_context(struct dsh_context *context);

int dsh_context_import_module(const char *module_filename, struct dsh_context *context);
int dsh_context_import_source(const char *source_file, struct dsh_context *context);

int dsh_context_exec_func(struct dsh_context *context, const char *name, const dsh_var *in_registers, dsh_var *out_registers);


#endif