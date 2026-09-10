#ifndef CLZ_FP_H
#define CLZ_FP_H

#include <stdint.h>

/*
 * clz64_fp(x): number of leading zero bits of x (0..64).
 *
 * Fills x upward so every bit at or below the top set bit is 1,
 * then reads the unbiased exponent of (double)x from the IEEE-754
 * binary64 bit layout. The conversion-rounding behavior for filled
 * values >= 2^53 is corrected exactly (see clz_fp.c and PROOF.md).
 *
 * x == 0 returns 64 by contract.
 */
uint64_t clz64_fp(uint64_t x);

#endif
