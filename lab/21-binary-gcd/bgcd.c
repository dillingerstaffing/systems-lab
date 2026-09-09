#include "bgcd.h"

/*
 * Binary GCD rests on three identities about divisibility by 2.
 * For unsigned u, v:
 *
 *   1. If both are even,  gcd(u, v) = 2 * gcd(u/2, v/2).
 *      Dividing by 2 is a right shift; this step only strips factors
 *      the two operands share.
 *
 *   2. If exactly one is even, the factor of 2 in that operand cannot
 *      divide the odd one, so it cannot be part of the gcd:
 *      gcd(u, v) = gcd(u/2, v) when u is even and v is odd.
 *
 *   3. If both are odd, their difference is even, and any common
 *      divisor of u and v divides v - u, so gcd(u, v) = gcd(u, v - u).
 *      Replacing the larger with the difference shrinks the pair, and
 *      the new even number gets its trailing zeros stripped by rule 2.
 *
 * Every iteration strictly reduces u + v once rule 1's shifts are done,
 * because v - u < v when u > 0. The loop ends when v reaches 0, and the
 * invariant gcd(original u, original v) = 2^shift * gcd(u, v) holds
 * through every step, so returning u << shift is exact.
 *
 * All arithmetic is unsigned, so no signed overflow or undefined
 * behavior is possible. Shifts are by 1 bit on uint32_t, and the final
 * u << shift uses shift < 32 (at least one operand is nonzero on entry,
 * so the common trailing-zero count cannot reach 32).
 */
uint32_t bgcd32(uint32_t u, uint32_t v)
{
    unsigned shift;

    if (u == 0u)
        return v;
    if (v == 0u)
        return u;

    /* Rule 1: strip the common factors of 2. */
    shift = 0u;
    while (((u | v) & 1u) == 0u) {
        u >>= 1;
        v >>= 1;
        ++shift;
    }

    /* Rule 2: u is even here, v is odd; strip u's factor of 2. */
    while ((u & 1u) == 0u)
        u >>= 1;

    do {
        /* Rule 2: strip v's trailing zeros, keeping v > 0 and odd. */
        while ((v & 1u) == 0u)
            v >>= 1;

        /* Rule 3: both odd; replace the larger with v - u (even, nonzero
         * unless u == v, in which case v becomes 0 and we are done). */
        if (u > v) {
            uint32_t t = u;
            u = v;
            v = t;
        }
        v = v - u;
    } while (v != 0u);

    return u << shift;
}
