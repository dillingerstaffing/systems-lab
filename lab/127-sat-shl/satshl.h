#ifndef SATSHL_H
#define SATSHL_H

#include <stdint.h>

/*
 * sat_shl64(x, k): x shifted left by k, saturating to UINT64_MAX on
 * overflow.
 *
 * Overflow means bits are shifted out of the top: for k in 1..63 the
 * bits shifted out are x >> (64 - k), so overflow iff that is nonzero.
 * For k == 0 the result is x. For k >= 64 the result is UINT64_MAX when
 * x != 0 and 0 when x == 0 (a C shift by >= 64 is undefined behavior,
 * so no shift is ever performed with an amount of 64 or more).
 */
uint64_t sat_shl64(uint64_t x, unsigned k);

#endif
