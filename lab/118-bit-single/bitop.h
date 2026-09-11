/* Single-bit set/clear/toggle/test of a 64-bit word, built from the
 * (1ULL << i) mask identities.
 *
 * A 64-bit unsigned integer is the sum of b_i * 2^i for i = 0..63, with
 * each b_i in {0, 1}. The mask 1ULL << i selects exactly bit i:
 *
 *   set:    x | m   (bit i forced to 1, every other bit unchanged)
 *   clear:  x & ~m  (bit i forced to 0, every other bit unchanged)
 *   toggle: x ^ m   (bit i flipped, every other bit unchanged)
 *   test:   (x >> i) & 1 (right shift moves bit i to position 0)
 *
 * The position i must lie in 0..63. Bit 63 is the most significant bit;
 * 1ULL << 63 is 0x8000000000000000 and is well defined (shift count 63 is
 * below the width of 64). A shift by 64 is undefined behavior in C
 * (C11 6.5.7p3), so each operation asserts i >= 0 && i < 64. The assert
 * is live in every test build below (no NDEBUG), the UBSan build arms the
 * shift-exponent check as a second tripwire, and a constant shift of 64
 * cannot even compile: -Werror (with -Wshift-count-overflow from -Wall)
 * rejects it.
 */
#ifndef BIT_SINGLE_H
#define BIT_SINGLE_H

#include <assert.h>
#include <stdint.h>

static uint64_t bset(uint64_t x, int i)
{
    assert(i >= 0 && i < 64); /* i = 63 is the MSB, the largest valid mask */
    return x | (1ULL << i);
}

static uint64_t bclr(uint64_t x, int i)
{
    assert(i >= 0 && i < 64);
    return x & ~(1ULL << i);
}

static uint64_t btg(uint64_t x, int i)
{
    assert(i >= 0 && i < 64);
    return x ^ (1ULL << i);
}

static int btst(uint64_t x, int i)
{
    assert(i >= 0 && i < 64);
    return (int)((x >> i) & 1ULL);
}

#endif /* BIT_SINGLE_H */
