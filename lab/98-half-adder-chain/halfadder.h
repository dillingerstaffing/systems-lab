#ifndef HALFADDER_H
#define HALFADDER_H

#include <stdint.h>

/* add_carry_chain(a, b, carry_in, &sum)
 *
 * Computes the 65-bit exact value a + b + carry_in, where a and b are
 * 64-bit words and carry_in is 0 or 1, by evaluating one full-adder
 * stage per bit position, from bit 0 up to bit 63.
 *
 * Per-bit stage (full adder built from two half adders):
 *   sum_i    = a_i ^ b_i ^ carry_in
 *   carry_{i+1} = (a_i & b_i) | (a_i & carry_in) | (b_i & carry_in)
 *
 * where a_i, b_i are the i-th bits of a and b. The carry-out of bit
 * i is the carry-in of bit i+1; the carry-out of bit 63 is the
 * function result.
 *
 * Writes the low 64 bits of the exact result to *sum and returns the
 * final carry-out: 1 iff a + b + carry_in >= 2^64 in exact arithmetic,
 * else 0. Equivalently, the 65-bit result is
 *   (carry_out << 64) | sum.
 *
 * The implementation uses only bitwise operations and shifts on
 * unsigned 64-bit values. No addition instruction on the operands is
 * used anywhere in the implementation; the native + operator appears
 * only in the test file as the independent oracle.
 */
uint64_t add_carry_chain(uint64_t a, uint64_t b, uint64_t carry_in,
                         uint64_t *sum);

/* add_carry_chain_w(a, b, carry_in, width, &sum)
 *
 * Same bit-serial construction, but over the low `width` bits only
 * (1 <= width <= 64). Only the low `width` bits of a and b are read.
 * Returns the carry-out of bit (width - 1); *sum holds the low
 * `width` bits of the exact a + b + carry_in. Used by the exhaustive
 * test over all 16-bit pairs. add_carry_chain is add_carry_chain_w
 * with width = 64.
 */
uint64_t add_carry_chain_w(uint64_t a, uint64_t b, uint64_t carry_in,
                           unsigned width, uint64_t *sum);

#endif
