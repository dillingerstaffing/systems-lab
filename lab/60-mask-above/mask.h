#ifndef MASK_ABOVE_H
#define MASK_ABOVE_H

#include <stdint.h>

/*
 * mask_above64(n): the n high bits of a 64-bit word set.
 *
 * For 1 <= n <= 64 the mask is built from the identity ~0ULL << (64 - n),
 * which leaves exactly the top n bits set: n = 1 gives
 * 0x8000000000000000, n = 64 gives ~0ULL << 0 = ~0ULL.
 * n = 0 gives the empty mask, 0.
 *
 * The n = 0 case is taken by the ternary branch, so the shift operand
 * (64 - n) is never 64: a 64-bit shift of a 64-bit value is undefined in
 * C, and no such shift ever executes. n > 64 is outside the contract
 * of this function and is not handled.
 */
static inline uint64_t mask_above64(int n)
{
    return (n == 0) ? 0ULL : (~0ULL << (64 - n));
}

#endif
