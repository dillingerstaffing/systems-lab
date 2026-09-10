#include "cmp.h"

/*
 * bcmp64(a, b) from the borrow-out identity, no conditional branch.
 *
 * Work entirely in the unsigned domain, where subtraction and
 * comparison are total (C11 6.2.5p9: unsigned arithmetic wraps
 * modulo 2^64).
 *
 * Fact 1 (borrow-out): for unsigned a, b,
 *   (a - b) mod 2^64 > a   iff   a < b.
 * Proof. If a >= b, then 0 <= a - b <= a, so (a - b) mod 2^64 = a - b
 * <= a. If a < b, then (a - b) mod 2^64 = 2^64 - (b - a); since
 * 1 <= b <= 2^64 - 1 and a <= 2^64 - 1, we have
 * 2^64 - (b - a) - a = 2^64 - b >= 1, i.e. the difference exceeds a.
 * The two cases partition all inputs, so the biconditional holds
 * for every uint64_t a, b, including the extremes.
 *
 * Fact 2: a == b iff (a - b) mod 2^64 == 0, because |a - b| < 2^64
 * and only 0 is congruent to 0 mod 2^64 in that range. (Used here
 * directly as a == b; the point is that both flags are computed
 * from total unsigned operations.)
 *
 * Fact 3: lt and eq cannot both be 1 (a < b and a == b are mutually
 * exclusive), so gt = 1 - lt - eq is in {0,1} and equals the
 * a > b flag: the three flags partition the inputs, exactly one of
 * them is 1, and result = gt - lt is -1, 0, or +1 accordingly.
 * (int64_t)gt and (int64_t)lt are 0 or 1, so their difference is
 * confined to {-1, 0, +1}; no signed overflow is possible.
 *
 * The comparisons compile to setcc-style flag reads, not branches;
 * the -O2 disassembly excerpt in PROOF.md shows no conditional jump.
 */
int64_t bcmp64(uint64_t a, uint64_t b)
{
    uint64_t d = a - b;        /* wraps modulo 2^64, total */
    uint64_t lt = d > a;       /* borrow-out: 1 iff a < b */
    uint64_t eq = a == b;      /* 1 iff a == b */
    uint64_t gt = 1 - lt - eq; /* 1 iff a > b, by Fact 3 */
    return (int64_t)gt - (int64_t)lt;
}
