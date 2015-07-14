#ifndef dash_vm_h
#define dash_vm_h

#include <stdint.h>

struct dsh_lib;

int dsh_load_lib(struct dsh_lib **lib, const char *dib_file);

int dsh_exec_func(struct dsh_lib *lib, uint32_t function_index, const uint32_t *in_registers, uint32_t *out_registers);

void dsh_destroy_lib(struct dsh_lib *lib);

#endif