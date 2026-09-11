#ifndef ISPOW2_H
#define ISPOW2_H

#include <stdint.h>

/*
 * is_pow2(x) = (x != 0) && ((x & (x - 1)) == 0), written branchless.
 *
 * A power of two has exactly one bit set, so clearing its lowest set bit
 * with x & (x - 1) leaves zero. The x != 0 guard is required because
 * (0 & (0 - 1)) == 0 would otherwise report true for 0.
 * Subtraction on x = 0 wraps mod 2^64 (unsigned arithmetic), which is why
 * the guard exists.
 */
int is_pow2(uint64_t x);

#endif /* ISPOW2_H */
