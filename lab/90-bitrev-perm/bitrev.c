#include "bitrev.h"

/* 3-bit reversal built only from the swap identities: exchange bit 0
 * with bit 2, leave bit 1 fixed. For a 3-bit value x = b2 b1 b0 this
 * yields b0 b1 b2. No lookup table, no library call. */
static unsigned bitrev3(unsigned x) {
	unsigned lo = (x >> 0) & 1u; /* bit 0, moves to position 2 */
	unsigned mid = x & 2u;       /* bit 1, stays */
	unsigned hi = (x >> 2) & 1u; /* bit 2, moves to position 0 */

	return (lo << 2) | mid | hi;
}

void bitrev8_perm(const uint32_t in[8], uint32_t out[8]) {
	for (unsigned i = 0; i < 8; i++) {
		out[bitrev3(i)] = in[i];
	}
}
