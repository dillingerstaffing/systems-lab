#ifndef MASK_BELOW_H
#define MASK_BELOW_H

#include <stdint.h>

/*
 * mask_below(n): the low-n-bit mask, built from the identity 2^n - 1.
 *
 * For 0 <= n < 64, 2^n - 1 has exactly the low n bits set and no others:
 * n = 0 gives 1 - 1 = 0, n = 63 gives 0x7FFFFFFFFFFFFFFF.
 * For n = 64 the mask is all 64 bits set, i.e. ~0ULL.
 *
 * The n = 64 case is handled by the ternary branch, so no 64-bit shift
 * is ever executed (a 64-bit shift of a 64-bit value is undefined in C).
 * n > 64 is outside this function's contract and is not handled.
 */
static inline uint64_t mask_below(int n)
{
    return (n == 64) ? ~0ULL : ((1ULL << n) - 1);
}

#endif
