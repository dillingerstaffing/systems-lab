#ifndef ROUND_RNE_H
#define ROUND_RNE_H

#include <stdint.h>

/*
 * Round a 64-bit Q32.32 fixed-point value to the nearest integer,
 * ties to even, from bit-manipulation identities only. No float,
 * no division anywhere in this translation unit.
 *
 * Bits: [63:32] integer part, [31:0] fraction part.
 *   round_bit = fraction bit 31 (value exactly 1/2)
 *   sticky    = OR of fraction bits 0..30 (1 iff frac != 0 and != 1/2)
 *   lsb       = integer bit 0
 *   round_up  = round_bit AND (sticky OR lsb)
 *   result    = integer_part + round_up, computed with unsigned
 *               32-bit addition, so 0xFFFFFFFF.FFFFFFFF rounds up
 *               and wraps to 0 (unsigned wraparound contract).
 */
uint32_t q32_32_round_even(uint64_t v);

#endif
