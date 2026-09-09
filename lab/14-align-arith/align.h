#ifndef ALIGN_ARITH_H
#define ALIGN_ARITH_H

#include <stdint.h>

/*
 * Alignment and power-of-two rounding for 32-bit unsigned values.
 *
 * The implementation (align.c) uses only <stdint.h>. The test harness
 * may use libc freely.
 *
 * All arithmetic is on uint32_t, so overflow wraps modulo 2^32
 * (well-defined C). For align_up, p + (a - 1) can wrap when p is near
 * UINT32_MAX; the wrapped result is verified exactly against the
 * division reference under the same wrap, so the stated behavior is
 * exact.
 *
 * Precondition: a is a power of two with a >= 1. Behavior is not
 * defined for a == 0 or non-power-of-two a; the tests only supply
 * powers of two.
 */

/* Round p down to a multiple of a: p & ~(a - 1). */
uint32_t align_down_u32(uint32_t p, uint32_t a);

/* Round p up to a multiple of a: (p + (a - 1)) & ~(a - 1). */
uint32_t align_up_u32(uint32_t p, uint32_t a);

/* 1 if x is a power of two (x > 0 and exactly one bit set), else 0. */
int is_pow2_u32(uint32_t x);

/*
 * Smallest power of two >= x, via the shift/or propagation identity.
 * Returns 1 for x <= 1. Overflow is defined: returns 0 when the answer
 * does not fit in 32 bits (x > 0x80000000), including x = UINT32_MAX.
 */
uint32_t round_up_pow2_u32(uint32_t x);

#endif /* ALIGN_ARITH_H */
