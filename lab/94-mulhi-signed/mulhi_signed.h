#ifndef MULHI_SIGNED_H
#define MULHI_SIGNED_H

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

/*
 * mulhi_s64: the high 64 bits of the SIGNED 128-bit product a * b.
 *
 * Let ua = (uint64_t)a, ub = (uint64_t)b (the same bit patterns read
 * as unsigned), sa = (a < 0), sb = (b < 0).  As integers,
 *     a = ua - sa * 2^64,    b = ub - sb * 2^64,
 * so
 *     a*b = ua*ub - sa*2^64*ub - sb*2^64*ua + sa*sb*2^128.
 * Every term is a multiple of 2^64 except the low word of ua*ub, so
 * the high 64 bits are
 *     high(a*b) = mulhi_u64(ua,ub) - sa*ub - sb*ua + sa*sb*2^64,
 * and the last term vanishes mod 2^64.  The true high word satisfies
 * |high| <= 2^62 (since |a*b| <= 2^126), so the value computed in
 * wrapped 64-bit unsigned arithmetic and read back as int64_t is the
 * exact signed high word.
 *
 * This is where the signed case differs from the unsigned 32-bit
 * splitting identity: the sign corrections subtract the full unsigned
 * operand whenever that operand is negative, which is the exact
 * contribution of the high half's sign extension (a negative operand
 * carries an implicit -2^64, whose product with the other operand
 * lands entirely in the high word).
 *
 * The corrections are conditional subtracts; the only multiplies are
 * the four 32-bit x 32-bit unsigned partial products in mulhi_u64.
 * No 128-bit integer type and no full-width signed multiply appear
 * here; the wide signed type is used only in the test's exact oracle
 * (test_mulhi_signed.c).
 */
static int64_t mulhi_s64(int64_t a, int64_t b)
{
    uint64_t ua = (uint64_t)a;
    uint64_t ub = (uint64_t)b;
    uint64_t hi = mulhi_u64(ua, ub);
    if (a < 0)
        hi -= ub;
    if (b < 0)
        hi -= ua;
    return (int64_t)hi;
}

#endif /* MULHI_SIGNED_H */
