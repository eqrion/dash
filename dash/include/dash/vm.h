#ifndef dash_vm_h
#define dash_vm_h

#include <stdint.h>

struct dsh_obj;

int dsh_load_obj(struct dsh_obj **obj, const char *dob_file);

int dsh_exec_obj(struct dsh_obj *obj, uint32_t function_index, const uint32_t *in_registers, uint32_t *out_registers);

void dsh_destroy_obj(struct dsh_obj *obj);

#endif