#include "xor_swap.h"

void xor_swap(uint64_t *a, uint64_t *b)
{
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}
