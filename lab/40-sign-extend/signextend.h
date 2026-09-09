#ifndef SIGNEXTEND_H
#define SIGNEXTEND_H

#include <stdint.h>

/*
 * sign_extend64(x, w): treat the low w bits of x as a signed w-bit
 * integer and return it as an int64_t.
 *
 * Contract: 1 <= w <= 64. w == 0 is outside the contract (it would
 * require a 64-bit shift, which is undefined behavior in C).
 *
 * Construction: x is shifted left so that bit (w-1) of x lands on
 * bit 63, then the result is shifted arithmetically right by the
 * same amount. Arithmetic right shift on a signed type replicates
 * the sign bit, so every bit position above (w-1) ends up equal to
 * the original bit (w-1). The shift amount 64 - w is in [0, 63],
 * so no shift ever reaches the undefined 64-bit case. For w == 64
 * both shifts are by 0 and the value is returned unchanged.
 */
static inline int64_t sign_extend64(uint64_t x, int w)
{
    uint64_t k = (uint64_t)(64 - w);
    return (int64_t)(x << k) >> (int64_t)k;
}

#endif /* SIGNEXTEND_H */
