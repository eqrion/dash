#ifndef dash_hash_h
#define dash_hash_h

#include <stdint.h>

// default values recommended by http://isthe.com/chongo/tech/comp/fnv/
#define DSH_HASH_PRIME 0x01000193 //   16777619
#define DSH_HASH_SEED 0x811C9DC5 // 2166136261

inline uint32_t dsh_hash(const char* text)
{
	uint32_t hash = DSH_HASH_SEED;
	while (*text)
		hash = (((unsigned char)*text++) ^ hash) * DSH_HASH_PRIME;
	return hash;
}

#endif