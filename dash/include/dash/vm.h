#ifndef dash_vm_h
#define dash_vm_h

#include <stdint.h>

#include "var.h"

struct dsh_lib;

int dsh_load_lib(struct dsh_lib **lib, const char *dib_file);

int dsh_exec_func(struct dsh_lib *lib, uint32_t function_index, const dsh_var *in_registers, dsh_var *out_registers);

void dsh_destroy_lib(struct dsh_lib *lib);

#endif