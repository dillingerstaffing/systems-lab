#include "roundup.h"

uint64_t round_up_pow2_64(uint64_t x) {
	/* All operations are on unsigned 64-bit values, so every step
	 * has well-defined wrap-around semantics. */
	uint64_t v = x - 1u;

	/* Fill cascade: after k = 1,2,4,8,16,32 the highest set bit of
	 * (x - 1) has been copied into all 64 lower bit positions, so v
	 * is of the form 2^m - 1 where 2^m is the smallest power of two
	 * above x (or 2^64 - 1 when x > 2^63). */
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;

	return v + 1u;
}
