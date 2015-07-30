#ifndef dash_vm_h
#define dash_vm_h

#include <stdint.h>
#include <stdio.h>

#include "var.h"

struct dsh_context;
struct dsh_function_def;

int dsh_create_context(struct dsh_context **context, size_t initial_function_capacity, size_t initial_bytecode_capacity);
void dsh_destroy_context(struct dsh_context *context);

int dsh_context_import_module(const char *module_filename, struct dsh_context *context);
int dsh_context_import_source(const char *source_file, struct dsh_context *context);

struct dsh_function_def *dsh_context_find_func(const char *name, struct dsh_context *context);

void dsh_context_dissasm_func(struct dsh_function_def *function, FILE *out, struct dsh_context *context);
int dsh_context_exec_func(struct dsh_function_def *function, const dsh_var *in_registers, dsh_var *out_registers, struct dsh_context *context);

#endif