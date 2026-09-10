#include "tie.h"

uint32_t tie_to_even_half(uint32_t x) {
	uint32_t q = x >> 1u;
	uint32_t tie = x & 1u;
	/* Round a tie up only when q is odd; an even q stays. */
	return q + (tie & (q & 1u));
}
