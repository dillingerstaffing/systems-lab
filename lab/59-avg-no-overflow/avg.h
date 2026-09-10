/* avg.h - average of two uint64_t values computed without overflow.
 *
 * avg_u64(a, b) returns floor((a + b) / 2) without ever forming a + b.
 *
 * The identity used: for each bit position i, the input bit pair
 * (a_i, b_i) contributes a_i + b_i to the sum. a_i & b_i is 1 exactly
 * when the pair contributes 2 (a carry into position i + 1), and
 * a_i ^ b_i is 1 exactly when the pair contributes 1. Summing over all
 * positions gives the ordinary integer equality
 *
 *     a + b = ((a & b) << 1) + (a ^ b).
 *
 * Halving both sides: (a + b) / 2 = (a & b) + (a ^ b) / 2. Since (a & b)
 * is an integer, the floors satisfy
 *
 *     floor((a + b) / 2) = (a & b) + floor((a ^ b) / 2)
 *                        = (a & b) + ((a ^ b) >> 1).
 *
 * Overflow analysis: (a & b) and ((a ^ b) >> 1) are each at most
 * 2^64 - 1, and their sum equals floor((a + b) / 2) <= 2^64 - 1 exactly,
 * so the final addition cannot wrap. All operands are unsigned, so there
 * is no signed overflow and no implementation-defined shift anywhere.
 */
#ifndef AVG_H
#define AVG_H

#include <stdint.h>

static inline uint64_t avg_u64(uint64_t a, uint64_t b)
{
    return (a & b) + ((a ^ b) >> 1);
}

#endif /* AVG_H */
