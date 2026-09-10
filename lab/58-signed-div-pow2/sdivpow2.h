#ifndef SDIVPOW2_H
#define SDIVPOW2_H

#include <stdint.h>

/* sdiv_trunc_pow2(x, k): the same value C computes for x / 2^k
 * (truncation toward zero), from the shift identity
 *   (x + ((x >> 63) & (2^k - 1))) >> k.
 *
 * Why the bias is needed: an arithmetic right shift of a negative value
 * rounds toward negative infinity (floor), while C division rounds toward
 * zero. For x < 0, write x = q * 2^k + r with q = floor(x / 2^k) and
 * 0 <= r < 2^k. Adding the bias 2^k - 1 gives
 *   x + 2^k - 1 = (q + 1) * 2^k + (r - 1)   when r > 0,
 *   x + 2^k - 1 = q * 2^k + (2^k - 1)       when r = 0,
 * so the arithmetic shift right by k yields q + 1 when r > 0 and q when
 * r = 0. In both cases that is exactly trunc(x / 2^k). For x >= 0 the
 * bias is 0 and x >> k is already the truncated quotient.
 *
 * The (x < 0) ? mask : 0 below is (x >> 63) & (2^k - 1) under two's
 * complement with an arithmetic shift, written with defined operations.
 *
 * No overflow: for x >= 0 the bias is 0; for x < 0 and k <= 63,
 * x + (2^k - 1) lies in [-2^63, 2^63 - 2], so the signed addition is
 * exact. The final shift is the arithmetic shift gcc documents; the
 * UBSan build confirms no undefined behavior is triggered.
 *
 * Edge cases:
 *   k = 0: division by 1, returns x unchanged (no shift executed).
 *   k >= 64: |x| <= 2^63 < 2^k, so the truncated quotient is exactly 0.
 */
static inline int64_t sdiv_trunc_pow2(int64_t x, unsigned k)
{
    if (k == 0) {
        return x;
    }
    if (k >= 64) {
        return 0;
    }
    uint64_t mask = ((uint64_t)1 << k) - (uint64_t)1; /* 2^k - 1 */
    int64_t bias = (x < 0) ? (int64_t)mask : 0;
    return (x + bias) >> k;
}

#endif /* SDIVPOW2_H */
