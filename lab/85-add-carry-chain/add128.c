#include "add128.h"

void add128(uint64_t a_hi, uint64_t a_lo,
            uint64_t b_hi, uint64_t b_lo,
            uint64_t carry_in,
            uint64_t *sum_hi, uint64_t *sum_lo,
            uint64_t *carry_out)
{
    /* Low stage: lo_sum = (a_lo + b_lo + carry_in) mod 2^64.
     * (lo_sum < a_lo) is the carry-out identity for one addition; the
     * second term catches the wrap through all-ones: when
     * a_lo + b_lo = 2^64 - 1 and carry_in = 1, lo_sum wraps back to
     * exactly a_lo (the a_lo = 0, b_lo = 2^64 - 1 case gives
     * lo_sum = 0), where the bare identity reports 0 and the true
     * carry is 1. Derived in PROOF.md. */
    uint64_t lo_sum = a_lo + b_lo + carry_in;
    uint64_t c_lo = (lo_sum < a_lo) | (carry_in & (lo_sum == a_lo));

    /* High stage: the same formula, with the low carry as carry_in
     * (c_lo is always 0 or 1, an OR of two 0/1 terms). */
    uint64_t hi_sum = a_hi + b_hi + c_lo;
    uint64_t c_out = (hi_sum < a_hi) | (c_lo & (hi_sum == a_hi));

    *sum_lo = lo_sum;
    *sum_hi = hi_sum;
    *carry_out = c_out;
}
