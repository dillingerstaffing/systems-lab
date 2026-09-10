#include "sign.h"

/*
 * sign64 via two sign-bit extractions, no comparison and no branch.
 *
 * Fact 1: for a two's-complement int64, (uint64_t)x >> 63 is 1 exactly
 * when x < 0 and 0 otherwise. The int64 -> uint64 conversion is
 * value-preserving modulo 2^64 (C11 6.3.1.3p2), and a right shift of
 * an unsigned value is a logical shift with zero fill (C11 6.5.7p5).
 * Negating that bit as an int64 gives -1 for negative x, 0 otherwise.
 *
 * Fact 2: unsigned subtraction wraps modulo 2^64 (C11 6.2.5p9), so
 * (0u - (uint64_t)x) is the unsigned value of -x even when
 * x = INT64_MIN, where the signed negation -x would be undefined
 * behavior. Its top bit is 1 exactly when x > 0:
 *   x = 0 -> 0u - 0 = 0, top bit 0
 *   x > 0 -> 0u - (uint64_t)x = 2^64 - x lies in [2^63+1, 2^64-1],
 *            top bit 1
 *   x < 0 -> top bit is 1 only for x = INT64_MIN (2^64 - 2^63 = 2^63),
 *            0 for every other negative x; either way this case is
 *            decided by Fact 1 below.
 * Note: in 0u - ux the 0u promotes to uint64_t, so the subtraction
 * is 64-bit unsigned wraparound, fully defined.
 *
 * The two cases are disjoint and cover all x: when x < 0, neg is -1
 * (all bits set), so neg | pos = -1 no matter what pos is; when
 * x >= 0, neg is 0 and the result is pos, which is 1 exactly for
 * x > 0 and 0 for x = 0. Hence the return is -1, 0, or +1 as
 * required, for every int64 including INT64_MIN and INT64_MAX.
 */
int64_t sign64(int64_t x)
{
    uint64_t ux = (uint64_t)x;
    int64_t neg = -(int64_t)(ux >> 63);
    int64_t pos = (int64_t)((0u - ux) >> 63);
    return neg | pos;
}
