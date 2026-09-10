/*
 * swar_abs.h - branchless absolute value of four packed signed
 * 16-bit lanes in a 64-bit word, using only word-level integer ops.
 *
 * Lanes sit in bits [0:16), [16:32), [32:48), [48:64).
 *
 * swar_abs_u16x4(x): lane i of the return value is abs(x_i), the
 * signed 16-bit absolute value, computed with no conditional
 * jump anywhere in the per-lane path.
 *
 * Contract: abs(-32768) = -32768 (0x8000).  There is no positive
 * 16-bit counterpart of -32768, so the wrap is pinned here: the
 * result is the two's-complement negation of the lane mod 2^16,
 * and for 0x8000 that negation wraps back to 0x8000.  The test
 * suite pins this case explicitly.
 *
 * Construction, in three steps.  The mechanism is one thing: the
 * sign-mask identity abs = (x ^ m) - m evaluated per lane with a
 * guard bit that makes inter-lane borrows impossible, plus a
 * final mask that strips the injected guard bit.
 *
 * Step 1: the per-lane sign mask.  m starts as x & H, the sign
 * bit of each lane, then
 *
 *   m |= m >> 1;  m |= m >> 2;  m |= m >> 4;  m |= m >> 8;
 *
 * Invariant: before each OR-shift, lane i of m holds the sign
 * predicate in bits 15 down to 15 - cov (cov the coverage so
 * far) and 0 below.  Bit 15 of lane i of (m >> k) is old lane
 * (i + 1)'s bit (k - 1); for k in {1, 2, 4, 8} that bit sits
 * below lane (i + 1)'s set run, so it is 0 (and for lane 3 the
 * source bits are above bit 63, also 0).  Every newly set bit is
 * the lane's own predicate copied downward, so no bit ever
 * crosses a lane boundary.  After the cascade, lane i of m is
 * 0xFFFF when x_i < 0 and 0x0000 otherwise.
 *
 * Step 2: the guarded subtraction.  Let H = 0x8000800080008000,
 * one guard bit per lane (bit 15).  Compute
 *
 *   t = ((x ^ m) | H) - (m & ~H)
 *
 * as a single 64-bit unsigned subtraction.  First, a_i = x_i ^ m_i
 * always has bit 15 clear: when m_i = 0, x_i is non-negative so
 * its bit 15 is 0; when m_i = 0xFFFF, x_i is negative so its
 * bit 15 is 1 and the XOR clears it.  Hence the minuend lane
 * (a_i | 0x8000) lies in [0x8000, 0xFFFF] and the subtrahend lane
 * (m_i & 0x7FFF) is 0 or 0x7FFF.  A borrow out of lane i would
 * require minuend < subtrahend + borrow_in <= 0x7FFF + 1 = 0x8000
 * <= minuend, which is impossible.  By induction (lane 0's
 * borrow_in is 0) the borrow into every lane is 0, so no borrow
 * ever leaves a lane and each lane subtracts in isolation:
 * t_i = (a_i | 0x8000) - (m_i & 0x7FFF), zero crosstalk.
 *
 * Step 3: strip the injected guard bit.  For a non-negative lane
 * (m_i = 0): t_i = x_i + 0x8000 and the true per-lane value
 * (x_i ^ 0) - 0 = x_i is recovered by clearing bit 15.  For a
 * negative lane (m_i = 0xFFFF): a_i = 0xFFFF - x_i lies in
 * [0, 0x7FFF], so t_i = a_i + 0x8000 - 0x7FFF = a_i + 1, which is
 * exactly (x_i ^ m_i) - m_i mod 2^16, the two's-complement
 * negation of the lane; its bit 15 is genuine result data.
 * Masking with (m | ~H) clears bit 15 exactly in the
 * non-negative lanes (m_i = 0 gives 0x7FFF) and keeps every bit
 * in the negative lanes (m_i = 0xFFFF gives 0xFFFF):
 *
 *   r = t & (m | ~H)
 *
 * Lane i of r is therefore (x_i ^ m_i) - m_i mod 2^16, which is
 * x_i for x_i >= 0 and the two's-complement negation of x_i for
 * x_i < 0: the absolute value, with 0x8000 mapping to itself.
 *
 * No builtins, no intrinsics, no inline asm, no library calls
 * anywhere; plain C11 unsigned arithmetic, shifts, and bitwise
 * ops.  Every shift count is a compile-time constant below 64.
 */
#ifndef SWAR_ABS_H
#define SWAR_ABS_H

#include <stdint.h>

/* One guard bit per 16-bit lane: bit 15 of each lane. */
#define SWAR_ABS_GUARD UINT64_C(0x8000800080008000)

/*
 * Per-lane signed 16-bit absolute value of a packed word.
 * Lane i of the return value is abs(x_i); abs(-32768) = -32768
 * (0x8000) by the pinned wrapping contract.
 */
static inline uint64_t swar_abs_u16x4(uint64_t x)
{
    /* Step 1: expand the per-lane sign bit to a full-lane mask.
     * 0xFFFF in lanes where x_i < 0, 0x0000 elsewhere; the
     * cascade cannot move a bit across a lane boundary (see the
     * invariant in the header comment). */
    uint64_t m = x & SWAR_ABS_GUARD;
    m |= m >> 1;
    m |= m >> 2;
    m |= m >> 4;
    m |= m >> 8;

    /* Step 2: guarded subtraction; lanes cannot exchange borrows
     * (see the guard-bounds proof in the header comment), so
     * lane i of t is (x_i ^ m_i) - m_i mod 2^16 plus an injected
     * 0x8000 exactly in the non-negative lanes. */
    uint64_t t = ((x ^ m) | SWAR_ABS_GUARD) - (m & ~SWAR_ABS_GUARD);

    /* Step 3: strip the injected guard bit where it was added. */
    return t & (m | ~SWAR_ABS_GUARD);
}

#endif /* SWAR_ABS_H */
