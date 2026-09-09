#ifndef BIT_REVERSE_H
#define BIT_REVERSE_H

#include <stdint.h>

/*
 * bit_reverse(x): reverse the order of all 64 bits of x; bit k of the
 * input moves to position 63 - k of the output.
 *
 * Method: repeated group swap, doubling the group size each step. Step
 * k takes the value as adjacent groups of 2^(k-1) bits and exchanges
 * the two halves of every group of 2^k bits:
 *
 *   x = ((x >> m) & mask) | ((x & mask) << m)
 *
 * with m = 1, 2, 4, 8, 16, 32 and mask selecting the low half of each
 * group. After the m = 32 step, bit k of the input sits at position
 * 63 - k. All arithmetic is unsigned, so no signed overflow is
 * possible and no shift is out of range. No compiler builtin or
 * dedicated instruction is used in the implementation; the byte
 * reordering in steps 4-6 is expressed with plain shifts and ORs.
 */
uint64_t bit_reverse(uint64_t x);

#endif
