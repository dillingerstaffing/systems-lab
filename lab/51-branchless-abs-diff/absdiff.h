/* absdiff.h — branchless absolute difference of two int64_t values.
 *
 * badiff64(a, b) returns |a - b| as a uint64_t. The result is the
 * sign-mask identity (d ^ (d >> 63)) - (d >> 63) applied to the
 * difference d = a - b, evaluated entirely on unsigned 64-bit values:
 * d is formed as (uint64_t)a - (uint64_t)b (well-defined wraparound),
 * and m = -(d >> 63) is the unsigned equivalent of the arithmetic
 * shift (d >> 63): all ones when d is negative, zero otherwise. Then
 * (d ^ m) - m is |d|, because for negative d (bit pattern 2^64 - k)
 * it computes ((2^64 - k) ^ (2^64 - 1)) - (2^64 - 1) = k mod 2^64,
 * and for non-negative d it is (d ^ 0) - 0 = d.
 *
 * Contract: defined exactly when the true difference a - b satisfies
 * INT64_MIN < (a - b) <= INT64_MAX, i.e. |a - b| <= INT64_MAX. When the
 * true difference fits, the wrapped difference reinterpreted as signed
 * is the true d, and the identity above is exact. d == INT64_MIN is
 * excluded: |INT64_MIN| is not representable as an int64_t, the signed
 * negation in the identity would overflow (undefined behavior), and the
 * reference llabs(INT64_MIN) is undefined too. Callers must not pass
 * pairs whose true difference is INT64_MIN.
 */
#ifndef ABSDIFF_H
#define ABSDIFF_H

#include <stdint.h>

static inline uint64_t badiff64(int64_t a, int64_t b)
{
    uint64_t d = (uint64_t)a - (uint64_t)b; /* wraps modulo 2^64 */
    uint64_t m = -(d >> 63); /* all ones iff d < 0 (as signed), else 0 */
    return (d ^ m) - m; /* == (d ^ (d >> 63)) - (d >> 63) on the mask */
}

#endif
