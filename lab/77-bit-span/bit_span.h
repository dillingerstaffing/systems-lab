/*
 * bit_span.h - bit_span(x): the position of the highest set bit of a
 * 64-bit word minus the position of the lowest set bit.
 *
 * Construction uses two bit identities, both rebuilt here from scratch:
 *
 * floor_log2(x), the index of the highest set bit, from the shift/OR
 * fill propagation identity.  The cascade
 *
 *   y = x;  y |= y >> 1;  y |= y >> 2;  y |= y >> 4;
 *   y |= y >> 8;  y |= y >> 16;  y |= y >> 32;
 *
 * turns every bit at or below the highest set bit of x into 1: after
 * the shifts, y is a run of (p + 1) ones where p is the highest set
 * bit index, so popcount(y) - 1 == p.  The popcount is the SWAR
 * sequence (pairwise sums in 2-bit fields, nibble sums, byte sums,
 * then the multiply accumulates the eight byte counts into the top
 * byte):
 *
 *   z = z - ((z >> 1) & 0x5555555555555555);
 *   z = (z & 0x3333333333333333) + ((z >> 2) & 0x3333333333333333);
 *   z = (z + (z >> 4)) & 0x0F0F0F0F0F0F0F0F;
 *   z = (z * 0x0101010101010101) >> 56;
 *
 * Contract: x != 0.  On x = 0 the cascade maps 0 to 0 and the
 * popcount minus 1 would underflow, which is meaningless; the caller
 * (bit_span) handles 0 separately.
 *
 * ctz(x), the index of the lowest set bit, from the de Bruijn
 * multiply-and-table identity.  Isolating the lowest set bit with
 * x & -x gives the single-bit word 2^k; multiplying it by the
 * 64-bit de Bruijn constant 0x03F79D71B4CB0A89 permutes the 64
 * single-bit words onto distinct top-6-bit values (the test
 * derives the table from this rule independently and checks the
 * hardcoded table below against it), and the shift >> 58 reads
 * the permutation key:
 *
 *   unsigned ctz = CTZ64_TABLE[((x & -x) * 0x03F79D71B4CB0A89) >> 58];
 *
 * Contract: x != 0.  On x = 0, x & -x is 0 and the multiply cannot
 * index a meaningful entry, so the caller handles 0 separately.
 *
 * bit_span(x) = floor_log2(x) - ctz(x) for x != 0.  For x = 0 the
 * span is defined to be 0: there are no set bits, so the distance
 * between the highest and lowest set bit is vacuously 0.  This is
 * a documented contract row, pinned by the anchors in the test
 * and by the exhaustive 16-bit pass (which includes 0).
 *
 * No builtins, no intrinsics, no inline asm, no library calls
 * anywhere; plain C11 shifts, masks, and unsigned arithmetic.
 */
#ifndef BIT_SPAN_H
#define BIT_SPAN_H

#include <stdint.h>

/* De Bruijn index table for the 64-bit constant 0x03F79D71B4CB0A89,
 * shift 58.  Entry i answers: which single-bit word 2^k permutes to
 * top-6-bit value i.  The test recomputes this table from the rule
 * (1ULL << k) * C >> 58 and asserts every entry matches. */
static const uint8_t CTZ64_DEBRUIJN[64] = {
     0,  1, 48,  2, 57, 49, 28,  3,
    61, 58, 50, 42, 38, 29, 17,  4,
    62, 55, 59, 36, 53, 51, 43, 22,
    45, 39, 33, 30, 24, 18, 12,  5,
    63, 47, 56, 27, 60, 41, 37, 16,
    54, 35, 52, 21, 44, 32, 23, 11,
    46, 26, 40, 15, 34, 20, 31, 10,
    25, 14, 19,  9, 13,  8,  7,  6,
};

/* Highest set bit index.  Requires x != 0. */
static inline unsigned floor_log2_u64(uint64_t x)
{
    /* Fill cascade: all bits at or below the highest set bit
     * become 1, giving a run of (p + 1) ones. */
    uint64_t y = x;
    y |= y >> 1;
    y |= y >> 2;
    y |= y >> 4;
    y |= y >> 8;
    y |= y >> 16;
    y |= y >> 32;

    /* SWAR popcount of the run, from scratch. */
    uint64_t z = y;
    z = z - ((z >> 1) & UINT64_C(0x5555555555555555));
    z = (z & UINT64_C(0x3333333333333333))
        + ((z >> 2) & UINT64_C(0x3333333333333333));
    z = (z + (z >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
    z = (z * UINT64_C(0x0101010101010101)) >> 56;

    return (unsigned)(z - 1);
}

/* Lowest set bit index.  Requires x != 0. */
static inline unsigned ctz_u64(uint64_t x)
{
    /* Isolate the lowest set bit: two's-complement identity. */
    uint64_t lowbit = x & (0u - x);

    /* De Bruijn permutation of the single-bit word onto the
     * top 6 bits, read off the table. */
    return CTZ64_DEBRUIJN[(lowbit * UINT64_C(0x03F79D71B4CB0A89)) >> 58];
}

/* Position of the highest set bit minus position of the lowest
 * set bit.  bit_span(0) is defined as 0 (documented contract). */
static inline unsigned bit_span(uint64_t x)
{
    if (x == 0)
        return 0;
    return floor_log2_u64(x) - ctz_u64(x);
}

#endif /* BIT_SPAN_H */
