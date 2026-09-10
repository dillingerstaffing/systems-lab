#ifndef SUBBORROW_H
#define SUBBORROW_H

#include <stdint.h>

/* sub_borrow_chain(a_hi, a_lo, b_hi, b_lo, borrow_in, &diff_hi, &diff_lo)
 *
 * Computes the 128-bit unsigned difference d = a - b - borrow_in, where
 * a = (a_hi << 64) | a_lo and b = (b_hi << 64) | b_lo.
 *
 * Writes the low and high 64 bits of d (mod 2^128) to *diff_lo and
 * *diff_hi, and returns the final borrow-out: 1 if a < b + borrow_in in
 * exact arithmetic, else 0.
 *
 * borrow_in must be 0 or 1. All intermediate arithmetic is on unsigned
 * 64-bit values, so every wrap is well defined by C11. No __int128
 * anywhere in the implementation; the oracle in the test file is the
 * only place __int128 appears.
 */
uint64_t sub_borrow_chain(uint64_t a_hi, uint64_t a_lo,
                          uint64_t b_hi, uint64_t b_lo,
                          uint64_t borrow_in,
                          uint64_t *diff_hi, uint64_t *diff_lo);

#endif
