#include "round_rne.h"

/*
 * Case analysis on the fraction bits (each fraction bit k has
 * value 2^(k-32); the integer part is exact):
 *
 *  - round_bit = 0: fraction < 1/2, since the top fraction bit
 *    alone is worth 1/2 and all remaining bits sum to < 1/2
 *    (bits 0..30 sum to at most 2^31 - 1, i.e. < 2^31 units of
 *    2^-32). So round_up = 0, result = integer_part. Correct.
 *
 *  - round_bit = 1, sticky = 1: fraction > 1/2, since it is
 *    1/2 plus at least one lower bit. So round_up = 1, result =
 *    integer_part + 1. Correct.
 *
 *  - round_bit = 1, sticky = 0: fraction is exactly 1/2. The two
 *    candidates are integer_part and integer_part + 1; exactly
 *    one of them is even, and it is integer_part when lsb = 0,
 *    integer_part + 1 when lsb = 1. So round_up = lsb picks the
 *    even one. Correct.
 *
 * All three cases are covered by round_up = round_bit AND
 * (sticky OR lsb), which is the single expression below. The
 * final addition is unsigned 32-bit: integer_part = 0xFFFFFFFF
 * with round_up = 1 yields 0, the pinned wraparound contract.
 */
uint32_t q32_32_round_even(uint64_t v)
{
    uint32_t ip = (uint32_t)(v >> 32);
    uint32_t fr = (uint32_t)v;
    uint32_t round_bit = (fr >> 31) & 1u;
    uint32_t sticky = (fr & 0x7FFFFFFFu) != 0u;
    uint32_t lsb = ip & 1u;
    uint32_t round_up = round_bit & (sticky | lsb);
    return ip + round_up;
}
