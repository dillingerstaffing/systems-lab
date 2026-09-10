#include "clz_fp.h"

/*
 * IEEE-754 binary64 layout: 1 sign bit, 11 exponent bits with
 * bias 1023, 52 stored mantissa bits. Bit 63 is the sign,
 * bits 62..52 are the exponent field, bits 51..0 the mantissa.
 */

static uint64_t double_bits(double d)
{
    union {
        double d;
        uint64_t u;
    } u;
    u.d = d;
    return u.u;
}

uint64_t clz64_fp(uint64_t x)
{
    uint64_t n;
    uint64_t bits;

    if (x == 0)
        return 64;

    /*
     * Fill downward from the top set bit. After the six ORs every
     * bit at or below the top set bit is 1. For x > 0 with
     * n = floor(log2(x)), the filled value is exactly 2^(n+1) - 1.
     */
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;

    /*
     * Read the exponent field of (double)x. The cast rounds to
     * nearest, ties to even, per the ambient rounding mode.
     */
    bits = double_bits((double)x);
    n = ((bits >> 52) & 0x7FFu) - 1023u;

    /*
     * Correction for conversion rounding, derived in PROOF.md.
     * For filled values >= 2^53 the cast to double is not exact:
     *   n = 53: the filled value is 2^54 - 1, an exact midpoint
     *     between the doubles 2^54 - 2 (significand 1.111...1, 52
     *     ones, least bit odd) and 2^54 (significand 1.0, least
     *     bit even). Round-ties-to-even picks the even one, 2^54,
     *     whose exponent field reads 54 = n + 1. One too high.
     *   n >= 54: the filled value is closer to 2^(n+1) (distance 1)
     *     than to the next lower double (distance 2^(n-52) - 1,
     *     which is >= 3), so the cast yields 2^(n+1) and the
     *     exponent field reads n + 1. One too high.
     * In every case with n >= 53 the field reads exactly one too
     * high, never more (the rounded value is at most 2^(n+1)).
     * Filling only sets bits at or below the top set bit, so
     * n >= 53 on the filled value iff the original x >= 2^53,
     * tested here as (x >> 53) != 0.
     */
    if (x >> 53)
        n -= 1;

    return 63 - n;
}
