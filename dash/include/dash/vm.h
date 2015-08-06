#ifndef dash_vm_h
#define dash_vm_h

#include <stdint.h>
#include <stdio.h>

#include "var.h"

struct dvm_context;
struct dvm_procedure;

int dvm_create_context(struct dvm_context **context, size_t initial_function_capacity, size_t initial_bytecode_capacity);
void dvm_destroy_context(struct dvm_context *context);

int dvm_import_module(const char *module_filename, struct dvm_context *context);
int dvm_import_source(const char *source_file, struct dvm_context *context);

struct dvm_procedure *dvm_find_proc(const char *name, size_t in_registers, size_t out_registers, struct dvm_context *context);

void dvm_dissasm_proc(struct dvm_procedure *function, FILE *out, struct dvm_context *context);
int dvm_exec_proc(struct dvm_procedure *function, const dvm_var *in_registers, dvm_var *out_registers, struct dvm_context *context);

#endif