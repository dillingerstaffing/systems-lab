/*
 * ceil_log2.h - ceil_log2_64(x): the smallest n with 2^n >= x,
 * for a 64-bit word x >= 1.
 *
 * Construction, from three bit identities:
 *
 *   1. floor_log2 from the shift/OR fill propagation identity:
 *
 *        y = x;  y |= y >> 1;  y |= y >> 2;  y |= y >> 4;
 *        y |= y >> 8;  y |= y >> 16;  y |= y >> 32;
 *
 *      Each stage doubles the downward reach of every 1 bit, and
 *      1+2+4+8+16+32 = 63 covers the whole word, so after the
 *      cascade every bit at or below the highest set bit of x is
 *      1.  Fill only turns 0s into 1s at or below an existing 1,
 *      so no bit above the top set bit changes.  For x >= 1 with
 *      top set bit at position f, y then has exactly bits f..0
 *      set: a popcount of f + 1.  Counting y with the SWAR
 *      popcount below and subtracting 1 gives floor_log2(x) = f.
 *      The SWAR steps:
 *
 *        z = z - ((z >> 1) & 0x5555555555555555);
 *        z = (z & 0x3333333333333333) +
 *            ((z >> 2) & 0x3333333333333333);
 *        z = (z + (z >> 4)) & 0x0F0F0F0F0F0F0F0F;
 *        z = (z * 0x0101010101010101) >> 56;
 *
 *      pairwise sums in disjoint 2-bit fields, nibble sums in
 *      disjoint 4-bit fields, byte sums in disjoint 8-bit fields
 *      (each byte count is at most 8, so no cross-byte carry),
 *      then the multiply accumulates the eight byte counts into
 *      the top byte.
 *
 *   2. Power-of-two test: (x & (x - 1)) == 0.  Subtracting 1
 *      borrows through the trailing zeros and clears the lowest
 *      set bit; AND-ing with x therefore removes exactly that
 *      one bit.  The result is 0 iff x had exactly one bit set,
 *      i.e. iff x is a power of two (for x >= 1).
 *
 *   3. ceil = floor + (x not a power of two ? 1 : 0).  If
 *      x = 2^f then floor = f and ceil = f; otherwise
 *      2^f < x < 2^(f+1) and the smallest n with 2^n >= x is
 *      f + 1.  Those two cases partition every x >= 1, so the
 *      sum is exact.
 *
 * Zero-input contract: the domain is x >= 1.  ceil_log2_64(0)
 * returns 0 by explicit contract; the guard sits before the fill
 * cascade because popcount(0) - 1 would wrap on the empty input.
 * The contract is pinned by a test row and kept out of the
 * differential set.
 *
 * No builtins, no intrinsics, no inline asm anywhere.  All
 * arithmetic is on unsigned 64-bit values, so every shift, add,
 * subtract, and multiply is fully defined.
 */
#ifndef CEIL_LOG2_H
#define CEIL_LOG2_H

#include <stdint.h>

/* floor_log2_64(x): position of the highest set bit, x >= 1. */
static inline unsigned floor_log2_64(uint64_t x)
{
    /* Fill cascade: propagate the highest set bit downward so
     * all bits at or below it are 1. */
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;

    /* x now has exactly floor_log2 + 1 bits set; count them. */
    uint64_t z = x;

    /* SWAR popcount, then minus 1. */
    z = z - ((z >> 1) & UINT64_C(0x5555555555555555));
    z = (z & UINT64_C(0x3333333333333333)) +
        ((z >> 2) & UINT64_C(0x3333333333333333));
    z = (z + (z >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
    z = (z * UINT64_C(0x0101010101010101)) >> 56;

    return (unsigned)z - 1u;
}

/* ceil_log2_64(x): smallest n with 2^n >= x.  Domain x >= 1;
 * ceil_log2_64(0) returns 0 by explicit contract. */
static inline unsigned ceil_log2_64(uint64_t x)
{
    if (x == 0)
        return 0;                       /* zero-input contract */
    unsigned f = floor_log2_64(x);      /* x >= 1 here */
    int pow2 = (x & (x - 1)) == 0;       /* exactly one bit set? */
    return f + (unsigned)(!pow2);
}

#endif /* CEIL_LOG2_H */
