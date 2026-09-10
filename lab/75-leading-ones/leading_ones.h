/*
 * leading_ones.h - clo64(x): the number of consecutive 1 bits starting
 * from the most significant bit of a 64-bit word.
 *
 * Construction, from the invert-then-scan identity:
 *
 *   clo(x) = clz(~x),
 *
 * because inverting maps leading ones of x onto leading zeros of ~x
 * one for one, in the same positions.  The leading-zero count is
 * built from the shift/OR fill cascade, no clz-class instruction:
 *
 *   y = ~x;  y |= y >> 1;  y |= y >> 2;  y |= y >> 4;
 *   y |= y >> 8;  y |= y >> 16;  y |= y >> 32;
 *
 * After the cascade, every bit at or below the highest set bit of
 * ~x is 1: if ~x has its top set bit at position p (bit 63 being
 * the most significant), the cascade fills bits 63..0 down to p
 * with 1s.  So ~y has exactly the 64 - p leading positions clear
 * and the rest set, i.e. ~y holds exactly clz(~x) one bits.  The
 * count comes from the SWAR popcount identities (sums of disjoint
 * bit pairs are exact, the final multiply accumulates the eight
 * byte counts into the top byte):
 *
 *   z = z - ((z >> 1) & 0x5555555555555555);   pairwise sums in 2-bit fields
 *   z = (z & 0x3333333333333333) + ((z >> 2) & 0x3333333333333333);
 *                                            nibble sums in 4-bit fields
 *   z = (z + (z >> 4)) & 0x0F0F0F0F0F0F0F0F;   byte sums in 8-bit fields
 *   z = (z * 0x0101010101010101) >> 56;        sum of the byte fields
 *
 * Zero-input contract: the degenerate input of the scan is ~x = 0,
 * i.e. x = 0xFFFFFFFFFFFFFFFF.  The cascade maps 0 to 0 and the
 * popcount of ~0 is 64, so clo(0xFFFFFFFFFFFFFFFF) = 64 with no
 * special case.  Note x = 0 itself has zero leading ones and
 * returns 0; the 64 case is the all-ones input, matching the usual
 * clz(0) = 64 convention applied to the inverted word.
 *
 * No builtins, no intrinsics, no inline asm anywhere.
 */
#ifndef LEADING_ONES_H
#define LEADING_ONES_H

#include <stdint.h>

static inline unsigned clo64(uint64_t x)
{
    /* Invert: leading ones of x become leading zeros of ~x. */
    uint64_t y = ~x;

    /* Fill cascade: propagate the highest set bit downward so all
     * bits at or below it are 1. */
    y |= y >> 1;
    y |= y >> 2;
    y |= y >> 4;
    y |= y >> 8;
    y |= y >> 16;
    y |= y >> 32;

    /* ~y now has exactly clz(~x) bits set; count them. */
    uint64_t z = ~y;

    /* SWAR popcount. */
    z = z - ((z >> 1) & UINT64_C(0x5555555555555555));
    z = (z & UINT64_C(0x3333333333333333)) +
        ((z >> 2) & UINT64_C(0x3333333333333333));
    z = (z + (z >> 4)) & UINT64_C(0x0F0F0F0F0F0F0F0F);
    z = (z * UINT64_C(0x0101010101010101)) >> 56;

    return (unsigned)z;
}

#endif /* LEADING_ONES_H */
