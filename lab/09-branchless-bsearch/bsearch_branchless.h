/* lab/09: branchless binary search over a sorted array of uint32_t.
 *
 * The search window is [lo, lo + len). Each iteration halves len. The
 * comparison a[mid] < key produces a 0/1 integer, and the next window is
 * chosen with integer arithmetic only, so the loop body has no
 * data-dependent branch. The loop always runs exactly
 * K = floor(log2(n)) + 1 iterations for a given n, for every key: paths
 * that narrow the window to empty early spend their remaining iterations
 * as no-ops (live = 0 forces lt = 0). There is no early exit: a hit costs
 * exactly the same as a miss.
 *
 * On success returns the index of the first element equal to key
 * (lower-bound semantics: with duplicate keys, the lowest index).
 * On failure returns -1.
 */
#ifndef BSEARCH_BRANCHLESS_H
#define BSEARCH_BRANCHLESS_H

#include <stddef.h>
#include <stdint.h>

static inline long bsearch_branchless(const uint32_t *a, size_t n,
                                      uint32_t key)
{
    /* Empty input: no candidate can exist. One-time boundary guard,
     * outside the measured search loop. */
    if (n == 0)
        return -1;

    /* Lower bound: on exit, lo is the first index i in [0, n] with
     * a[i] >= key (lo == n when no such index exists).
     *
     * Invariant at the top of each iteration: every index that can hold
     * key lies in [lo, lo + len), and lo + len <= n, so a[mid] is always
     * in bounds while the search is live (half < len, hence
     * mid = lo + half < lo + len <= n).
     *
     * Narrowing: if a[mid] < key, the window becomes [mid + 1, lo + len),
     * i.e. lo advances by half + 1 and len becomes len - half - 1;
     * otherwise the window becomes [lo, mid), i.e. len becomes half.
     * The 0/1 value lt selects between the two arithmetically:
     *   lo  = lo   + lt * (half + 1)
     *   len = half + lt * (len - 2 * half - 1)
     * When lt == 0 the parenthesized term is multiplied by zero, so its
     * unsigned wraparound is harmless (unsigned arithmetic wraps by
     * definition; it is never a trap or UB).
     *
     * Exactly K = floor(log2(n)) + 1 iterations run on every path. Each
     * iteration maps len to at most ceil(len / 2) (the two options are
     * half and len - half - 1, which differ by at most one), so after K
     * halvings len == 0 on every path; a path that reaches len == 0
     * early then iterates with live == 0, which forces lt = 0 and reads
     * a[0] instead of a[mid] (mid * live == 0, in bounds because n >= 1),
     * leaving lo and len untouched. Hence the trip count depends only
     * on n, never on the key or the array contents. */
    unsigned K = 0;
    for (size_t t = n; t > 0; t >>= 1)
        K++; /* K = floor(log2(n)) + 1; trip count depends only on n */
    size_t lo = 0;
    size_t len = n;
    for (unsigned i = 0; i < K; i++) {
        size_t half = len >> 1;
        size_t mid = lo + half;
        size_t live = (size_t)(len > 0);
        uint32_t v = a[mid * live];
        size_t lt = live & (size_t)(v < key);
        lo += lt * (half + 1);
        len = half + lt * (len - (half << 1) - 1);
    }

    /* Finalize without branching. lo <= n; if lo == n there is no
     * candidate. Reading a[lo * in] is always in bounds: when in == 1,
     * lo < n; when in == 0 and n >= 1, the read is a[0], which exists;
     * n == 0 returned above. hit is 1 exactly when a lower-bound
     * candidate exists and equals key. The return value is selected
     * arithmetically: hit * lo - (1 - hit), i.e. lo on hit, -1 on miss. */
    size_t in = (size_t)(lo < n);
    uint32_t v = a[lo * in];
    size_t hit = in & (size_t)(v == key);
    long r = (long)(lo * hit);
    r = (long)hit * r - (long)(1 - hit);
    return r;
}

#endif /* BSEARCH_BRANCHLESS_H */
