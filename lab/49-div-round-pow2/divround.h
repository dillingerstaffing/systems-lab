#ifndef DIVROUND_H
#define DIVROUND_H

#include <stdint.h>

/* round_half_up_div_pow2(x, k): nearest integer to x / 2^k, ties round up.
 *
 * The identity for 1 <= k <= 31:
 *   round(x / 2^k) = floor(x / 2^k + 1/2) = (x + 2^(k-1)) >> k.
 * Adding half the divisor before the shift moves the rounding boundary so
 * that a fractional part of exactly 1/2 (x = m * 2^k + 2^(k-1)) lands on
 * the upper integer; anything below the tie keeps its floor value.
 *
 * The addition runs in 64 bits because the 32-bit form can wrap: for
 * x = 0xFFFFFFFF and k = 31, (uint32_t)(x + 2^30) wraps and yields 0,
 * while the true value round((2^32 - 1) / 2^31) is 2. The 64-bit
 * intermediate keeps the same identity exact over the whole u32 domain.
 *
 * Edge cases, both handled without any invalid shift count:
 *   k = 0: division by 1, returns x unchanged (no shift executed).
 *   k >= 32: divisor 2^k exceeds the u32 range; computed as
 *     ((uint64_t)x + 2^31) >> k. For 32-bit x the exact quotient is
 *     below 1, so the result is 1 exactly at the tie x = 2^31 and 0
 *     otherwise. Shifts by 32 are valid on the 64-bit operand.
 */
static inline uint32_t round_half_up_div_pow2(uint32_t x, unsigned k)
{
    if (k == 0) {
        return x;
    }
    if (k >= 32) {
        return (uint32_t)(((uint64_t)x + ((uint64_t)1 << 31)) >> k);
    }
    return (uint32_t)(((uint64_t)x + ((uint64_t)1 << (k - 1))) >> k);
}

#endif /* DIVROUND_H */
