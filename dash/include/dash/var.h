#ifndef dash_var_h
#define dash_var_h

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

#endif
