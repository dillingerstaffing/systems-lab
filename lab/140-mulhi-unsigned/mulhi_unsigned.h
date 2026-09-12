#ifndef MULHI_UNSIGNED_H
#define MULHI_UNSIGNED_H

#include <stdint.h>

/*
 * mulhi_u64: the high 64 bits of the 128-bit product of two uint64_t
 * values, from the 32-bit splitting identity a = ah*2^32 + al,
 * b = bh*2^32 + bl:
 *
 *     a*b = ah*bh * 2^64 + (al*bh + ah*bl) * 2^32 + al*bl,
 *
 * so high(a*b) = p3 + (m1 << 32) + (mid >> 32) + c, where
 * p3 = ah*bh, mid = p1 + p2 (wrapped, m1 = 1 iff the add wrapped),
 * and c = 1 iff (mid << 32) + p0 wrapped.  Every partial product is
 * 32 bits x 32 bits, so it fits in 64 bits with no 128-bit type.
 *
 * No __int128 appears here: the 128-bit product is never formed.
 * The unsigned __int128 type is used only in the test's exact
 * reference (test_mulhi_unsigned.c).
 */
static uint64_t mulhi_u64(uint64_t a, uint64_t b)
{
    uint64_t al = (uint32_t)a;  /* low 32 bits of a */
    uint64_t ah = a >> 32;      /* high 32 bits of a */
    uint64_t bl = (uint32_t)b;
    uint64_t bh = b >> 32;

    uint64_t p0 = al * bl;      /* each partial product < 2^64 */
    uint64_t p1 = al * bh;
    uint64_t p2 = ah * bl;
    uint64_t p3 = ah * bh;

    uint64_t mid = p1 + p2;
    uint64_t m1 = (mid < p1);           /* 1 iff p1 + p2 wrapped */
    uint64_t s = (mid << 32) + p0;
    uint64_t c = (s < p0);              /* 1 iff (mid<<32) + p0 wrapped */

    return p3 + (m1 << 32) + (mid >> 32) + c;
}

#endif /* MULHI_UNSIGNED_H */
