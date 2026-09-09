#include "ctz.h"

/*
 * Counting trailing zeros with a de Bruijn multiplication.
 *
 * Two bit-level facts carry the whole algorithm:
 *
 *   1. For nonzero unsigned x, `x & -x` (with -x = ~x + 1 in two's
 *      complement) clears every bit except the lowest set one, so
 *      the result is exactly 2^k where k is the trailing-zero count.
 *      Proof sketch: -x flips all bits below the lowest 1 of x and
 *      keeps the lowest 1, so the AND with x keeps only that bit.
 *
 *   2. Multiplying each power 2^k (k = 0..31) by the constant
 *      0x077CB531 and keeping the top 5 bits yields 32 distinct
 *      values. Distinctness is a property of this particular
 *      constant (a de Bruijn sequence written into 32 bits); the
 *      test binary asserts it directly for every k, so the table
 *      below is checked, not trusted. The top 5 bits therefore act
 *      as a perfect hash of the single set bit's position, and the
 *      table inverts the hash back to k.
 *
 * x == 0 has no set bit, so the answer is defined as 32 by the
 * header contract (matching the naive reference used in testing).
 * All arithmetic is unsigned; the shift amounts (27) and the table
 * index (0..31) are in range, so there is no undefined behavior.
 */
#define CTZ_DEBRUIJN 0x077CB531u

static const unsigned char ctz_table[32] = {
    0u,  1u, 28u,  2u, 29u, 14u, 24u,  3u,
   30u, 22u, 20u, 15u, 25u, 17u,  4u,  8u,
   31u, 27u, 13u, 23u, 21u, 19u, 16u,  7u,
   26u, 12u, 18u,  6u, 11u,  5u, 10u,  9u
};

unsigned ctz32(uint32_t x)
{
    uint32_t lowbit;

    if (x == 0u)
        return 32u;

    lowbit = x & (0u - x);
    return ctz_table[(lowbit * CTZ_DEBRUIJN) >> 27];
}
