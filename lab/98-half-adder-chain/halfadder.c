#include "halfadder.h"

/* Bit-serial ripple-carry addition.
 *
 * The per-bit full adder is two half adders in sequence:
 *   half adder 1 on (a_i, b_i): p_i = a_i ^ b_i (partial sum),
 *                                  g_i = a_i & b_i (generate).
 *   half adder 2 on (p_i, carry_in): sum_i = p_i ^ carry_in,
 *                                      c2  = p_i & carry_in.
 * The carry-out is g_i | c2. Expanding c2 with p_i = a_i ^ b_i:
 *   c2 = (a_i ^ b_i) & carry_in
 *      = (a_i & ~b_i & carry_in) | (~a_i & b_i & carry_in)
 *      = (a_i & carry_in) | (b_i & carry_in),
 * since when exactly one of a_i, b_i is 1, (a_i ^ b_i) is 1 and the
 * AND with carry_in splits into the two terms; when a_i == b_i the
 * XOR is 0 and both forms agree (carry_in term 0). So the stage is
 * exactly:
 *   sum_i   = a_i ^ b_i ^ carry_in
 *   carry_{i+1} = (a_i & b_i) | (a_i & carry_in) | (b_i & carry_in),
 * which matches the identity in the header.
 *
 * Correctness of the ripple: carry_{i+1} is the true overflow of the
 * i-th bit column (it is 1 iff at least two of {a_i, b_i, carry_in}
 * are 1, i.e. the column sums to 2 or 3), so feeding it as the next
 * column's carry-in computes the exact binary addition. The carry out
 * of the final column is 1 iff the exact sum needs 65 bits.
 *
 * Only &, |, ^, shifts, and the loop counter i appear below; the
 * operands never pass through an addition. All values are unsigned, so every shift and wrap is defined by C11.
 */
uint64_t add_carry_chain_w(uint64_t a, uint64_t b, uint64_t carry_in,
                           unsigned width, uint64_t *sum)
{
	uint64_t carry = carry_in & 1u;
	uint64_t s = 0;
	unsigned i;

	for (i = 0; i < width; i++) {
		uint64_t ai = (a >> i) & 1u;
		uint64_t bi = (b >> i) & 1u;
		uint64_t si = ai ^ bi ^ carry;

		carry = (ai & bi) | (ai & carry) | (bi & carry);
		s |= si << i;
	}

	*sum = s;
	return carry;
}

uint64_t add_carry_chain(uint64_t a, uint64_t b, uint64_t carry_in,
                         uint64_t *sum)
{
	return add_carry_chain_w(a, b, carry_in, 64, sum);
}
