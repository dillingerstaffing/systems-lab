/*
 * swar_max.h - lane-wise and horizontal maximum of packed unsigned
 * 16-bit lanes in a 64-bit word, using only word-level integer ops.
 *
 * Lanes sit in bits [0:16), [16:32), [32:48), [48:64).
 *
 * swar_max2_u16x4(x, y): lane i of the result is max(x_i, y_i),
 * the unsigned 16-bit maximum, computed branchless.
 *
 * swar_max_u16x4(x): the maximum of the four lanes of x, returned
 * as a uint16_t.
 *
 * Construction of swar_max2_u16x4, in four steps.  The mechanism
 * is one thing: a per-lane unsigned comparison built from a
 * subtraction whose guard bits make inter-lane borrows
 * impossible, then a branchless select.
 *
 * Step 1: the guarded subtraction.  Let H = 0x8000800080008000,
 * one guard bit per lane (bit 15).  Compute
 *
 *   t = (x | H) - (y & ~H)
 *
 * as a single 64-bit unsigned subtraction.  In lane i the minuend
 * is (x_i | 0x8000), a value in [0x8000, 0xFFFF], and the
 * subtrahend is (y_i & 0x7FFF), a value in [0x0000, 0x7FFF].
 * A borrow out of lane i would require
 * minuend < subtrahend + borrow_in <= 0x7FFF + 1 = 0x8000,
 * but the minuend is always >= 0x8000, so no borrow ever leaves
 * a lane: by induction the borrow into every lane is 0 and each
 * lane subtracts in isolation.  Lane i of t is therefore exactly
 * 0x8000 + (x_i & 0x7FFF) - (y_i & 0x7FFF), a value in
 * [0x0001, 0xFFFF].  Bit 15 of lane i of t is 1 exactly when
 * (x_i & 0x7FFF) >= (y_i & 0x7FFF): a 15-bit unsigned comparison
 * with zero crosstalk, proved from the guard bounds above.
 *
 * Step 2: fold in bit 15.  Write hx_i, hy_i for bit 15 of x_i,
 * y_i and lx_i, ly_i for the low 15 bits.  Unsigned 16-bit
 * comparison decomposes as: x_i >= y_i exactly when
 * (hx_i > hy_i) or (hx_i == hy_i and lx_i >= ly_i).  With
 * m15 = t & H carrying the low-15 comparison at bit 15,
 *
 *   p = ((x & H) & ~(y & H)) | (~((x ^ y) & H) & m15)
 *
 * has bit 15 of lane i set exactly when x_i >= y_i.  The four
 * (hx_i, hy_i) cases: (0,0) and (1,1) reduce to m15 because
 * ~((x^y) & H) keeps bit 15 exactly when the top bits agree;
 * (1,0) is forced to 1 by the first term; (0,1) is forced to 0
 * because the first term is 0 and ~((x^y) & H) clears bit 15
 * exactly when the top bits differ.
 *
 * Step 3: expand the bit-15 predicate to a full-lane mask.
 * p has only bit 15 possibly set in each lane.  Run
 *
 *   m = p;  m |= m >> 1;  m |= m >> 2;  m |= m >> 4;  m |= m >> 8;
 *
 * Invariant: before each OR-shift, lane i of m holds the
 * predicate p_i in bits 15 down to 15 - cov (cov the coverage
 * so far) and 0 below.  A right shift by k < 16 cannot smuggle
 * a foreign bit into the lane: bit 15 of lane i of (m >> k) is
 * old bit (k - 1) of lane i + 1, and for k in {1, 2, 4, 8} that
 * bit sits below lane i + 1's set run, so it is 0; every other
 * newly set bit is p_i copied downward.  After the cascade,
 * lane i of m is 0xFFFF when x_i >= y_i and 0x0000 otherwise.
 *
 * Step 4: select.  out = (x & m) | (y & ~m) keeps x_i in lanes
 * where x_i >= y_i and y_i elsewhere, so lane i of the result
 * is max(x_i, y_i).
 *
 * swar_max_u16x4(x): m1 = swar_max2_u16x4(x, x >> 32) has lane 0
 * = max(lane 0, lane 2) and lane 1 = max(lane 1, lane 3);
 * m2 = swar_max2_u16x4(m1, m1 >> 16) has lane 0 = max of those
 * two, i.e. the maximum of all four lanes.  Return lane 0.
 *
 * No builtins, no intrinsics, no inline asm, no library calls
 * anywhere; plain C11 unsigned arithmetic, shifts, and bitwise
 * ops.  Every shift count is a compile-time constant below 64.
 */
#ifndef SWAR_MAX_H
#define SWAR_MAX_H

#include <stdint.h>

/* One guard bit per 16-bit lane: bit 15 of each lane. */
#define SWAR_MAX_GUARD UINT64_C(0x8000800080008000)

/*
 * Per-lane unsigned 16-bit maximum of two packed words.
 * Lane i of the return value is max(x_i, y_i).
 */
static inline uint64_t swar_max2_u16x4(uint64_t x, uint64_t y)
{
    /* Step 1: guarded subtraction; lanes cannot exchange borrows
     * (see the guard-bounds proof in the header comment), so
     * bit 15 of each lane of t compares the low 15 bits. */
    uint64_t t = (x | SWAR_MAX_GUARD) - (y & ~SWAR_MAX_GUARD);
    uint64_t m15 = t & SWAR_MAX_GUARD;

    /* Step 2: fold bit 15 into the comparison.  Bit 15 of lane i
     * of p is 1 exactly when x_i >= y_i (unsigned). */
    uint64_t p = ((x & SWAR_MAX_GUARD) & ~(y & SWAR_MAX_GUARD))
               | (~((x ^ y) & SWAR_MAX_GUARD) & m15);

    /* Step 3: expand the bit-15 predicate to a full-lane mask.
     * 0xFFFF in lanes where x_i >= y_i, 0x0000 elsewhere. */
    uint64_t m = p;
    m |= m >> 1;
    m |= m >> 2;
    m |= m >> 4;
    m |= m >> 8;

    /* Step 4: branchless per-lane select. */
    return (x & m) | (y & ~m);
}

/*
 * Horizontal maximum: the largest of the four unsigned 16-bit
 * lanes of x, as a uint16_t.
 */
static inline uint16_t swar_max_u16x4(uint64_t x)
{
    uint64_t m1 = swar_max2_u16x4(x, x >> 32);
    uint64_t m2 = swar_max2_u16x4(m1, m1 >> 16);
    return (uint16_t)m2;
}

#endif /* SWAR_MAX_H */
