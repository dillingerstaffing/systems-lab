#include "subborrow.h"

/* One 64-bit stage of the subtraction: returns the borrow-out of
 * r = a - b - bin (bin is 0 or 1), with *r = (a - b - bin) mod 2^64.
 *
 * Derivation (see PROOF.md): the exact borrow-out is 1 iff
 * a < b + bin as plain integers. Write t = (b + bin) mod 2^64, the
 * value the wrapping addition actually computes.
 *   - If t did not wrap (bin == 0, or b < 2^64 - 1), then t == b + bin
 *     exactly, so borrow-out is exactly (a < t).
 *   - If t wrapped, then bin == 1 and b == 2^64 - 1, so b + bin == 2^64
 *     exactly; every a satisfies a < 2^64, so borrow-out is 1. The wrap
 *     is detected by (t < b): wrapping makes t == 0 < b.
 * The difference word is a - t mod 2^64, which equals a - (b + bin)
 * mod 2^64 in both cases (in the wrap case, t == 0 and
 * (b + bin) mod 2^64 == 0).
 */
static uint64_t sub_stage(uint64_t a, uint64_t b, uint64_t bin, uint64_t *r)
{
	uint64_t t = b + bin;
	uint64_t borrow = (uint64_t)(a < t) | (bin & (uint64_t)(t < b));

	*r = a - t;
	return borrow;
}

uint64_t sub_borrow_chain(uint64_t a_hi, uint64_t a_lo,
                          uint64_t b_hi, uint64_t b_lo,
                          uint64_t borrow_in,
                          uint64_t *diff_hi, uint64_t *diff_lo)
{
	/* The borrow-out of the low stage is the borrow-in of the high
	 * stage; sub_stage returns exactly 0 or 1, so the chain contract
	 * (borrow_in in {0,1}) holds at the second call too. */
	uint64_t borrow_lo = sub_stage(a_lo, b_lo, borrow_in, diff_lo);
	uint64_t borrow_hi = sub_stage(a_hi, b_hi, borrow_lo, diff_hi);

	return borrow_hi;
}
