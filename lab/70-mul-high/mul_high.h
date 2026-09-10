#ifndef MUL_HIGH_H
#define MUL_HIGH_H

#include <stdint.h>

/*
 * mul_high64: the high 64 bits of the 128-bit product a * b.
 *
 * Write a = ah * 2^32 + al and b = bh * 2^32 + bl, each half below 2^32.
 * Then
 *     a * b = ah*bh * 2^64 + (al*bh + ah*bl) * 2^32 + al*bl,
 * so
 *     high(a*b) = ah*bh + floor((al*bh + ah*bl + (al*bl >> 32)) / 2^32).
 *
 * Every partial product fits in 64 bits (each factor is below 2^32).
 * The middle sum p1 + p2 + (p0 >> 32) can reach 2^65, so it is formed
 * in a 64-bit register with explicit wrap detection: (x + y) < x is 1
 * exactly when the addition wrapped.  With c1, c2 the two wrap bits,
 *
 *     floor((p1 + p2 + (p0 >> 32)) / 2^32)
 *         = (c1 + c2) * 2^32 + ((p1 + p2 + (p0 >> 32)) mod 2^64) >> 32,
 *
 * which is exact in 64-bit unsigned arithmetic.  Finally p3 + q cannot
 * wrap: p3 <= (2^32 - 1)^2 and q <= 2^33 - 3, so p3 + q < 2^64.
 *
 * No 128-bit type, no intrinsics, no builtins: only 64-bit unsigned
 * shifts, adds, multiplies, and comparisons, all well-defined.
 */
static inline uint64_t mul_high64(uint64_t a, uint64_t b)
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
    uint64_t c1 = (mid < p1);            /* 1 iff p1 + p2 wrapped */
    uint64_t mid2 = mid + (p0 >> 32);
    uint64_t c2 = (mid2 < (p0 >> 32));   /* 1 iff mid + (p0>>32) wrapped */

    uint64_t q = (c1 + c2) * ((uint64_t)1 << 32) + (mid2 >> 32);
    return p3 + q;
}

#endif /* MUL_HIGH_H */
