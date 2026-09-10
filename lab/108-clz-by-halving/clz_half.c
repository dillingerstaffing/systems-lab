#include "clz_half.h"

/*
 * Invariant proof. Let p be the position of the top set bit of x
 * (x != 0 here), so the true answer is 63 - p. Define
 * remaining = 63 - p - n, the number of leading zeros not yet
 * counted. Entering each step of half size k, n equals the total
 * shift already applied to x, so the top set bit of the shifted x
 * sits at position p + n.
 *
 * The step tests (x >> (64 - k)) == 0, i.e. the top k bits of the
 * shifted x are zero, i.e. p + n <= 63 - k, i.e. remaining >= k.
 *   - If true, those k bits are leading zeros: n += k and the
 *     shift x <<= k preserve the invariant, remaining drops by k.
 *   - If false, remaining < k, so the top set bit lies inside the
 *     top k bits; none of them is a leading zero and x is left
 *     alone.
 *
 * Shifting left by k only moves set bits upward; bits shifted out
 * were counted zeros, so nothing is lost.
 *
 * The half sizes 32, 16, 8, 4, 2, 1 sum to 63 and reduce any
 * remaining in 0..63 to 0 by the greedy halving (each step
 * subtracts k exactly when remaining >= k). Since
 * 63 - p <= 63, remaining ends at 0, hence n = 63 - p exactly.
 * The x = 0 case is pinned to 64 by contract before any step.
 *
 * All shifts are on uint64_t by constants in 1..63: no overflow,
 * no undefined behavior. No clz-class instruction appears in the
 * implementation; the test disassembles the -O2 object file and
 * fails if bsr, bsf, lzcnt, or tzcnt is present.
 */

uint64_t clz64_halving(uint64_t x)
{
    uint64_t n = 0;

    if (x == 0)
        return 64;

    if ((x >> 32) == 0) {
        n += 32;
        x <<= 32;
    }
    if ((x >> 48) == 0) {
        n += 16;
        x <<= 16;
    }
    if ((x >> 56) == 0) {
        n += 8;
        x <<= 8;
    }
    if ((x >> 60) == 0) {
        n += 4;
        x <<= 4;
    }
    if ((x >> 62) == 0) {
        n += 2;
        x <<= 2;
    }
    if ((x >> 63) == 0) {
        n += 1;
    }

    return n;
}
