/* median3.h — branchless median of three int64_t values.
 *
 * med3(a, b, c) returns the median of its three inputs, computed as a
 * 3-element sorting network: three compare-swap steps, each built from
 * a comparison mask and bitwise selection, with no conditional jumps.
 *
 * The comparison mask. Signed order maps to unsigned order by flipping
 * the sign bit: A = (uint64_t)a ^ SIGN. For the unsigned comparison
 * A < B, split into 32-bit halves (AH, AL) and (BH, BL). Each half
 * comparison is exact in 64-bit unsigned arithmetic because the
 * operands are under 2^32, so their difference can never wrap:
 *   hmask(xh, yh) = -(((xh - yh) >> 63))
 * is all ones exactly when xh < yh, else zero. With exactly one of
 * (xh < yh), (xh == yh), (xh > yh) holding, heq = ~(hlo | hhi) is all
 * ones exactly when the high halves are equal, and
 *   lt = hlo | (heq & llo)
 * is all ones exactly when (AH, AL) < (BH, BL) lexicographically, i.e.
 * exactly when A < B as unsigned integers. Feeding the sign-flipped
 * words makes this mask the signed predicate a < b, exact on the whole
 * int64_t range.
 *
 * Compare-swap. m = signed-lt-mask(a, b) selects one input bitwise:
 *   mn = b ^ ((a ^ b) & m), mx = b ^ ((a ^ b) & ~m)
 * so no subtraction of the inputs is ever performed and no overflow is
 * possible: the result is always one of the two inputs bit-for-bit.
 *
 * Median. The network (a,b), (max_ab,c), (min_ab, min_maxc):
 *   mn1 = min(a,b), mx1 = max(a,b)
 *   mn2 = min(mx1,c)
 *   med = max(mn1, mn2)
 * mn2 is min(max(a,b), c), so max(mn1, mn2) is the value that survives
 * in the middle position after the three swaps, i.e. the median.
 *
 * Contract: defined for every int64_t triple, including INT64_MIN and
 * INT64_MAX. There is no excluded input: every subtraction is on
 * 32-bit-zero-extended halves (never wraps), and the inputs are only
 * ever selected, never added or subtracted.
 */
#ifndef MEDIAN3_H
#define MEDIAN3_H

#include <stdint.h>

/* All-ones iff the 32-bit-zero-extended values satisfy x < y.
 * xh, yh, xl, yl must each be under 2^32 (true for x >> 32 and
 * (uint32_t)x), so xh - yh never wraps. */
static inline uint64_t u64_lt32_mask(uint64_t x, uint64_t y)
{
    return -(((x - y) >> 63));
}

/* All-ones iff x < y as unsigned 64-bit integers, else zero. */
static inline uint64_t u64_lt_mask(uint64_t x, uint64_t y)
{
    uint64_t xh = x >> 32;
    uint64_t yh = y >> 32;
    uint64_t xl = (uint64_t)(uint32_t)x;
    uint64_t yl = (uint64_t)(uint32_t)y;
    uint64_t hlo = u64_lt32_mask(xh, yh); /* xh < yh */
    uint64_t hhi = u64_lt32_mask(yh, xh); /* xh > yh */
    uint64_t heq = ~(hlo | hhi); /* xh == yh */
    uint64_t llo = u64_lt32_mask(xl, yl); /* xl < yl */
    return hlo | (heq & llo);
}

/* All-ones iff a < b as signed int64_t, else zero. XOR with the sign
 * bit maps two's-complement order onto unsigned order exactly. */
static inline uint64_t s64_lt_mask(int64_t a, int64_t b)
{
    return u64_lt_mask((uint64_t)a ^ 0x8000000000000000ull,
                       (uint64_t)b ^ 0x8000000000000000ull);
}

/* Branchless compare-swap: mn = min(a,b), mx = max(a,b). */
static inline void swap2(int64_t a, int64_t b, int64_t *mn, int64_t *mx)
{
    uint64_t au = (uint64_t)a;
    uint64_t bu = (uint64_t)b;
    uint64_t m = s64_lt_mask(a, b);
    *mn = (int64_t)(bu ^ ((au ^ bu) & m));
    *mx = (int64_t)(bu ^ ((au ^ bu) & ~m));
}

static inline int64_t med3(int64_t a, int64_t b, int64_t c)
{
    int64_t mn1, mx1, mn2, mx2;
    swap2(a, b, &mn1, &mx1);
    swap2(mx1, c, &mn2, &mx2);
    int64_t med, tmp;
    swap2(mn1, mn2, &tmp, &med); /* med = max(min(a,b), min(max(a,b),c)) */
    return med;
}

#endif
