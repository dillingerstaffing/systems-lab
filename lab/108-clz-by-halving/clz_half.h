#ifndef CLZ_HALF_H
#define CLZ_HALF_H

#include <stdint.h>

/*
 * clz64_halving(x): number of leading zero bits of x (0..64),
 * from the binary-search halving identity. Each step tests whether
 * the current top half of x is zero; if so it counts the half and
 * shifts x up by the half, so the next test again probes the top.
 *
 * Steps, with k the half size: if (x >> (64 - k)) == 0 then
 * n += k, x <<= k, for k = 32, 16, 8, 4, 2, 1.
 *
 * x == 0 returns 64 by contract.
 */
uint64_t clz64_halving(uint64_t x);

#endif
