/*
 * wrapneg.h: two's-complement negation of a 64-bit word from the
 * (~x + 1) identity only.
 *
 * There is no unary minus anywhere in this implementation.  The
 * arithmetic is entirely unsigned, so there is no undefined
 * behavior on any input, including 0x8000000000000000.
 *
 * Contract on the INT64_MIN edge: wrapneg(0x8000000000000000)
 * returns 0x8000000000000000 (it wraps to itself).  The two
 * algebraic invariants still hold there: x + wrapneg(x) == 0
 * (mod 2^64) and wrapneg(wrapneg(x)) == x, because adding the
 * word to itself under 64-bit wrap gives zero.
 */
#ifndef WRAPNEG_H
#define WRAPNEG_H

#include <stdint.h>

/* Negate x: NOT then increment, no unary minus. */
static inline uint64_t wrapneg_u64(uint64_t x)
{
    return ~x + 1u;
}

#endif
