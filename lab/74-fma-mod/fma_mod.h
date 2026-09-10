#ifndef FMA_MOD_H
#define FMA_MOD_H

#include <stdint.h>

/*
 * fma_mod64: the exact value (a * b + c) mod 2^64, built only from the
 * 32-bit splitting identities.  Write a = ah * 2^32 + al and
 * b = bh * 2^32 + bl, each half below 2^32.  Then
 *
 *     a * b = p3 * 2^64 + (p1 + p2) * 2^32 + p0,
 *
 * with the four 32x32 -> 64 partial products
 *     p0 = al*bl,  p1 = al*bh,  p2 = ah*bl,  p3 = ah*bh,
 * each exact in 64 bits because both factors are below 2^32.
 *
 * Group the middle column col1 = p1 + p2 + (p0 >> 32); col1 is an
 * integer below 2^65, and
 *
 *     a * b = p3 * 2^64 + col1 * 2^32 + (p0 mod 2^32).
 *
 * col1 is formed in a 64-bit register with explicit wrap detection:
 * (x + y) < x is 1 exactly when the addition wrapped, so with
 *
 *     mid  = p1 + p2,            k1 = (mid  < p1),
 *     mid2 = mid + (p0 >> 32),   k2 = (mid2 < (p0 >> 32)),
 *
 * col1 = (k1 + k2) * 2^64 + mid2 exactly.  (k1 + k2 <= 1: both bits
 * set would need col1 >= 2^65, impossible since
 * p1 + p2 <= 2*(2^32 - 1)^2 and (p0 >> 32) < 2^32.)
 *
 * Modulo 2^64, the terms p3 * 2^64 and (k1 + k2) * 2^96 vanish, and
 * mid2 * 2^32 mod 2^64 = ((uint32_t)mid2) * 2^32, so the low 64 bits
 * of a * b are exactly
 *
 *     ab_lo = (uint32_t)p0 + (((uint32_t)mid2) << 32),
 *
 * a sum below 2^64 ((uint32_t)p0 < 2^32 and
 * ((uint32_t)mid2 << 32) <= (2^32 - 1) * 2^32), hence formed with no
 * wrap possible.
 *
 * The high 64 bits are accumulated for exactness accounting as
 *
 *     ab_hi = p3 + (k1 + k2) * 2^32 + (mid2 >> 32),
 *
 * which equals floor(a * b / 2^64) by the column identity above.
 * Since a * b < 2^128, ab_hi < 2^64, and every partial sum is a
 * sub-sum of non-negative terms, no addition in ab_hi can wrap.
 * ab_hi is discarded: (a * b) mod 2^64 keeps only ab_lo.
 *
 * Finally c is added mod 2^64; unsigned 64-bit addition wraps, and
 * wraparound is exactly addition mod 2^64.
 *
 * No 128-bit type, no intrinsics, no builtins: only 64-bit unsigned
 * shifts, adds, multiplies, and comparisons, all well-defined.
 */
static inline uint64_t fma_mod64(uint64_t a, uint64_t b, uint64_t c)
{
    uint64_t al = (uint32_t)a;  /* low 32 bits of a */
    uint64_t ah = a >> 32;      /* high 32 bits of a */
    uint64_t bl = (uint32_t)b;  /* low 32 bits of b */
    uint64_t bh = b >> 32;      /* high 32 bits of b */

    /* Four 32x32 -> 64 partial products, each exact in 64 bits. */
    uint64_t p0 = al * bl;
    uint64_t p1 = al * bh;
    uint64_t p2 = ah * bl;
    uint64_t p3 = ah * bh;

    /* Middle column with explicit carry tracking. */
    uint64_t mid = p1 + p2;
    uint64_t k1 = (mid < p1);            /* 1 iff p1 + p2 wrapped */
    uint64_t mid2 = mid + (p0 >> 32);
    uint64_t k2 = (mid2 < (p0 >> 32));   /* 1 iff mid + (p0>>32) wrapped */

    /* Low 64 bits of a * b: exact, no wrap possible (see above). */
    uint64_t ab_lo = (uint32_t)p0 + (((uint64_t)(uint32_t)mid2) << 32);

    /* High 64 bits of a * b: exact, no wrap possible (see above).
       Discarded: only ab_lo survives mod 2^64. */
    uint64_t ab_hi = p3 + (k1 + k2) * ((uint64_t)1 << 32) + (mid2 >> 32);
    (void)ab_hi;

    /* Add c mod 2^64: unsigned wraparound is exactly mod-2^64 addition. */
    return ab_lo + c;
}

#endif /* FMA_MOD_H */
