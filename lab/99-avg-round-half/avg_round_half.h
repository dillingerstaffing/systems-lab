/* avg_round_half.h - round-half-up average of two uint64_t, no overflow.
 *
 * avg_rhu_u64(a, b) returns round_half_up((a + b) / 2) without ever
 * forming a + b.
 *
 * The arithmetic rests on one identity. For each bit position i, the
 * input bit pair (a_i, b_i) contributes a_i + b_i to the sum: 0, 1, or 2.
 * (a_i & b_i) is 1 exactly when the pair contributes 2 (a carry into
 * position i + 1), and (a_i ^ b_i) is 1 exactly when the pair
 * contributes 1. Summing over all positions gives the ordinary integer
 * equality, for the exact (unbounded) sum s = a + b:
 *
 *     s = 2 * (a & b) + (a ^ b).
 *
 * Write y = a & b and x = a ^ b, so s = 2y + x. Rounding half up is
 * rhu(q) = floor(q + 1/2). Then
 *
 *     rhu(s / 2) = floor(y + x/2 + 1/2) = y + floor(x/2 + 1/2)
 *
 * because y is an integer. For an integer x, floor(x/2 + 1/2) is
 * x >> 1 when x is even, and (x >> 1) + 1 when x is odd; in both
 * cases it is (x >> 1) + (x & 1). Hence
 *
 *     rhu((a + b) / 2) = (a & b) + ((a ^ b) >> 1) + ((a ^ b) & 1),
 *
 * where the last term adds 1 exactly when x is odd, i.e. exactly when
 * the exact sum s is odd (the .5 case), which is the tie that rounding
 * half up pushes to the ceiling.
 *
 * Overflow analysis: y + (x >> 1) equals floor((a + b) / 2) exactly
 * (the lab/59 decomposition), so it is at most 2^64 - 1 and that
 * addition cannot wrap. The correction (x & 1) is 0 or 1. When it is 1,
 * s is odd, and an odd sum satisfies s <= 2^65 - 3 because the maximum
 * sum 2^65 - 2 is even; thus floor(s / 2) <= 2^64 - 2 in that case and
 * the final +1 stays below 2^64. All operands are unsigned, so there
 * is no signed overflow and no implementation-defined shift anywhere.
 */
#ifndef AVG_ROUND_HALF_H
#define AVG_ROUND_HALF_H

#include <stdint.h>

static inline uint64_t avg_rhu_u64(uint64_t a, uint64_t b)
{
    uint64_t x = a ^ b;
    return (a & b) + (x >> 1) + (x & 1u);
}

#endif /* AVG_ROUND_HALF_H */
