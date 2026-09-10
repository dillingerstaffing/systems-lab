#include "hsum.h"

/*
 * swar_hsum(w): horizontal sum of four packed u16 lanes, from the
 * pairwise-add cascade identity with guard-bit spacing.
 *
 * Fact 1: unsigned addition is addition modulo 2^64 (C11 6.2.5p9), so
 * the integer sum is exact whenever it is known that no bit above
 * the highest live bit can be set. All lanes are non-negative, so
 * each lane pair sum is at most 65535 + 65535 = 131070 = 0x1FFFE,
 * which fits in 17 bits.
 *
 * Stage 1. Isolate the even lanes in place and the odd lanes shifted
 * down by 16:
 *   a = w & 0x0000FFFF0000FFFF   (lanes 0 and 2 in place)
 *   b = (w >> 16) & 0x0000FFFF0000FFFF   (lanes 1 and 3 aligned down)
 * Then t = a + b. Write lanes as 32-bit fields: field 0 of a holds
 * lane0, field 0 of b holds lane1. Their sum is lane0 + lane1 at most
 * 0x1FFFE, which is strictly less than 2^32. Because the field sum
 * cannot reach 2^32, it produces no carry into bit 32; bits 16..31
 * of the field (the guard bits, both zero in the addends) hold only
 * bit 16 of the field sum (at most 1) and zero above that. The same
 * holds for field 1 (lane2 + lane3). Therefore bit 48 and above of t
 * are all zero, bit [31:0] of t is exactly lane0+lane1, and bit
 * [63:32] (equivalently [47:32] plus a zero guard above) is exactly
 * lane2+lane3. No bit of one pair sum can enter the other pair's
 * field: the guard bound is the inequality lane sum < 2^32, proved
 * from the lane bound 0xFFFF.
 *
 * Stage 2. Fold the two 32-bit fields:
 *   lo = t & 0xFFFFFFFF          (= lane0+lane1, at most 0x1FFFE)
 *   hi = t >> 32                (= lane2+lane3, at most 0x1FFFE)
 *   out = lo + hi
 * Each addend is at most 0x1FFFE, so out is at most 0x3FFFC, well
 * below 2^32; the addition is exact in the low 32 bits and no carry
 * leaves bit 31. The result is (lane0+lane1) + (lane2+lane3), the
 * lane sum, returned as uint32_t.
 *
 * The crosstalk absence is this exact argument: at every stage, each
 * partial sum is proved strictly smaller than the power of two that
 * would reach the next field, so addition is lane-local. Nothing
 * here is approximate and no lane ever leaks into a neighbor.
 */
uint32_t swar_hsum(uint64_t w)
{
    uint64_t a = w & 0x0000FFFF0000FFFFULL;
    uint64_t b = (w >> 16) & 0x0000FFFF0000FFFFULL;
    uint64_t t = a + b;
    uint64_t lo = t & 0xFFFFFFFFULL;
    uint64_t hi = t >> 32;
    return (uint32_t)(lo + hi);
}
