#ifndef ADD128_H
#define ADD128_H

#include <stdint.h>

/* add128: 128-bit unsigned addition from 64-bit words.
 *
 * Computes s = (a_hi:a_lo) + (b_hi:b_lo) + carry_in as a 129-bit value:
 * *sum_hi:*sum_lo hold s mod 2^128 and *carry_out is floor(s / 2^128).
 *
 * carry_in must be 0 or 1. All arithmetic is unsigned 64-bit, so every
 * wrap is defined by C11. No 128-bit integer type is used here (the
 * test oracle may use one). Each stage applies the exact carry formula derived in
 * PROOF.md:
 *
 *   carry = (sum < a) | (carry_in & (sum == a))
 *
 * where sum = (a + b + carry_in) mod 2^64. */
void add128(uint64_t a_hi, uint64_t a_lo,
            uint64_t b_hi, uint64_t b_lo,
            uint64_t carry_in,
            uint64_t *sum_hi, uint64_t *sum_lo,
            uint64_t *carry_out);

#endif /* ADD128_H */
