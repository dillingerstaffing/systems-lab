#include "sat_neg.h"

/*
 * Derivation. Work in unsigned 64-bit arithmetic (mod 2^64), where
 * u = (uint64_t)x is the two's complement bit pattern of x, and
 * s = (uint64_t)(x >> 63) is the sign mask: 0 for x >= 0 and
 * 2^64 - 1 (all ones) for x < 0.
 *
 * Base identity (no edge correction yet): s - u + (s & 1).
 *   - x >= 0: s = 0, so the value is 0 - u + 0 = -u mod 2^64, the
 *     two's complement negation of x. Since -x lies in
 *     [-(2^63 - 1), 0], it is representable and the bit pattern
 *     is exactly -x.
 *   - x < 0, x != INT64_MIN: s = 2^64 - 1, s & 1 = 1, so the value
 *     is (2^64 - 1) - u + 1 = 2^64 - u = -u mod 2^64. Here
 *     -x lies in [1, 2^63 - 1], representable, so the pattern is
 *     exactly -x.
 *   - x == INT64_MIN: s = 2^64 - 1, u = 2^63, so the base gives
 *     (2^64 - 1) - 2^63 + 1 = 2^63, whose bit pattern is INT64_MIN
 *     again (the negation wrapped). We need INT64_MAX = 2^63 - 1,
 *     exactly one less.
 *
 * The edge is folded into the low mask bit: replace (s & 1) with
 * ((s & 1) ^ e), where e = 1 iff x == INT64_MIN. Then the MIN row
 * computes (2^64 - 1) - 2^63 + (1 ^ 1) = 2^63 - 1 = INT64_MAX,
 * and every other row is unchanged because e = 0 there.
 *
 * Branchless edge detector, no comparison: let MIN = 2^63 and
 * d = u ^ MIN, so d = 0 iff x == INT64_MIN. For d != 0, at least
 * one of d and -d mod 2^64 has bit 63 set: if bit 63 of d is 1
 * then d itself does; otherwise d < 2^63 and -d = 2^64 - d lies in
 * [2^63, 2^64 - 1], whose bit 63 is set. Hence
 * (d | (0 - d)) >> 63 is 1 iff d != 0, and 0 iff d = 0.
 * e = 1 - that value is 1 exactly for x == INT64_MIN.
 *
 * No signed overflow anywhere: every operation is on uint64_t.
 */

int64_t neg_sat64(int64_t x)
{
    uint64_t u = (uint64_t)x;
    uint64_t s = (uint64_t)(x >> 63);
    uint64_t d = u ^ 0x8000000000000000ULL;
    uint64_t e = 1u - ((d | (0u - d)) >> 63);
    uint64_t r = s - u + ((s & 1u) ^ e);

    return (int64_t)r;
}
