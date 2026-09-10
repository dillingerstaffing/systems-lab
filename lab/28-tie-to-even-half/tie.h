#ifndef TIE_H
#define TIE_H

#include <stdint.h>

/*
 * tie_to_even_half: round x / 2 to the nearest integer, with ties
 * (odd x) rounded to the even neighbor.
 *
 * Bit identity, computed directly from the bits of x:
 *   q = x >> 1, r = x & 1
 *   result = q + (r & (q & 1))
 * When x is even, r = 0 and the result is exactly q. When x is odd,
 * x = 2q + 1 is exactly halfway between q and q + 1; the tie is
 * resolved to the even of the two, so q rounds up only when q is
 * odd (then q + 1 is even).
 *
 * All operations are unsigned 32-bit: shifts, AND, and the final
 * add. No floats, no division. The result is always <= x.
 */
uint32_t tie_to_even_half(uint32_t x);

#endif
