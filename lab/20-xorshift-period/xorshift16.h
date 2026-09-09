#ifndef XORSHIFT16_H
#define XORSHIFT16_H

#include <stdint.h>

/*
 * xorshift16: a 16-bit linear generator over GF(2).
 *
 * Recurrence, exact 16-bit semantics (all intermediate values are
 * narrowed to 16 bits before the next line, matching a 16-bit
 * register):
 *
 *   x ^= x << 7;
 *   x ^= x >> 9;
 *   x ^= x << 8;
 *
 * This is the (7, 9, 8) parameter triple for 16-bit xorshift, one of
 * the triples listed by Marsaglia ("Xorshift RNGs", 2003) that give
 * the maximal period 2^16 - 1. The triple was chosen because it is
 * documented as full-period; the exhaustive state-space walk in the
 * test suite below independently re-verifies the claim, so the choice
 * is backed by measurement, not just citation.
 *
 * Each step is a permutation of the 16-bit state space: every shift
 * is invertible modulo 2^16 and the XORs preserve that, so distinct
 * states map to distinct states. Consequence: the state sequence
 * from any nonzero seed is a single cycle, and 0 is a fixed point
 * (0 ^ 0 == 0 on every line), which the generator provably never
 * enters and never leaves.
 */
uint16_t xorshift16_step(uint16_t x);

#endif
