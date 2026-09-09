/* Tests for lab/09-branchless-bsearch.
 *
 * Correctness: differential test against libc bsearch (used ONLY as the
 * reference oracle, never in the implementation). Unique arrays compare
 * exact index; duplicate arrays compare found/not-found and verify the
 * returned index is the first occurrence. Covers empty array, 1-element
 * array, boundary values 0 and UINT32_MAX, and duplicates.
 *
 * Iteration-count check: a counting twin of the algorithm verifies every
 * lookup runs exactly floor(log2(n)) + 1 iterations regardless of the
 * key, i.e. a hit costs the same as a miss (no early exit).
 *
 * Performance: cycles per lookup via rdtsc (lfence-serialized) for the
 * branchless search vs libc bsearch on identical key sets, hits and
 * misses separately. The benchmark key sets double as extra differential
 * cases (agreement is re-checked on every timed lookup).
 *
 * All randomness is a deterministic xorshift64 with a fixed seed, so
 * every run below is reproducible.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsearch_branchless.h"

/* ------------------------------------------------------------------ */
/* deterministic PRNG                                                  */
/* ------------------------------------------------------------------ */

static uint64_t rng_state = 0x123456789ABCDEFULL;

static uint64_t xorshift64(void)
{
    uint64_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}

/* ------------------------------------------------------------------ */
/* rdtsc timing (lfence-serialized)                                    */
/* ------------------------------------------------------------------ */

static inline uint64_t rdtsc_begin(void)
{
    unsigned hi, lo;
    __asm__ volatile("lfence\n\trdtsc"
                     : "=a"(lo), "=d"(hi)
                     :
                     : "memory");
    return ((uint64_t)hi << 32) | lo;
}

static inline uint64_t rdtsc_end(void)
{
    unsigned hi, lo;
    __asm__ volatile("rdtsc\n\tlfence"
                     : "=a"(lo), "=d"(hi)
                     :
                     : "memory");
    return ((uint64_t)hi << 32) | lo;
}

/* ------------------------------------------------------------------ */
/* oracle: libc bsearch, reference only                                 */
/* ------------------------------------------------------------------ */

static int cmp_u32(const void *p, const void *q)
{
    uint32_t x = *(const uint32_t *)p;
    uint32_t y = *(const uint32_t *)q;
    return (x > y) - (x < y);
}

/* oracle result as an index, or -1 on miss */
static long oracle_index(const uint32_t *a, size_t n, uint32_t key)
{
    const uint32_t *p =
        (const uint32_t *)bsearch(&key, a, n, sizeof(uint32_t), cmp_u32);
    if (p == NULL)
        return -1;
    return (long)(p - a);
}

/* ------------------------------------------------------------------ */
/* counting twin: same algorithm, counts loop iterations                */
/* ------------------------------------------------------------------ */

static long bsearch_counted(const uint32_t *a, size_t n, uint32_t key,
                            unsigned *iters)
{
    unsigned c = 0;
    long r;
    if (n == 0) {
        *iters = 0;
        return -1;
    }
    unsigned K = 0;
    for (size_t t = n; t > 0; t >>= 1)
        K++;
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
        c++;
    }
    size_t in = (size_t)(lo < n);
    uint32_t v = a[lo * in];
    size_t hit = in & (size_t)(v == key);
    r = (long)(lo * hit);
    r = (long)hit * r - (long)(1 - hit);
    *iters = c;
    return r;
}

static unsigned expected_iters(size_t n)
{
    unsigned c = 0;
    while (n > 0) {
        n >>= 1;
        c++;
    }
    return c; /* floor(log2(n)) + 1 for n >= 1 */
}

/* ------------------------------------------------------------------ */
/* test bookkeeping                                                    */
/* ------------------------------------------------------------------ */

static unsigned long long checks;
static unsigned long long mismatches;

static void fail(const char *what, size_t n, uint32_t key, long got,
                 long want)
{
    printf("MISMATCH %s: n=%zu key=%u got=%ld want=%ld\n", what, n, key,
           got, want);
    mismatches++;
    if (mismatches > 20) {
        printf("too many mismatches, aborting\n");
        exit(1);
    }
}

/* unique sorted array: exact index must match the oracle */
static void check_unique(const uint32_t *a, size_t n, uint32_t key)
{
    long got = bsearch_branchless(a, n, key);
    long want = oracle_index(a, n, key);
    checks++;
    if (got != want)
        fail("unique", n, key, got, want);
}

/* array with duplicates: found/not-found must match; on hit the index
 * must point at key and be the first occurrence (lower-bound rule) */
static void check_dups(const uint32_t *a, size_t n, uint32_t key)
{
    long got = bsearch_branchless(a, n, key);
    long want = oracle_index(a, n, key);
    checks++;
    if ((got < 0) != (want < 0)) {
        fail("dups-found", n, key, got, want);
        return;
    }
    if (got >= 0) {
        if ((size_t)got >= n || a[got] != key) {
            fail("dups-value", n, key, got, want);
            return;
        }
        if (got > 0 && a[got - 1] == key)
            fail("dups-first", n, key, got, want);
    }
}

static uint32_t *alloc_u32(size_t n)
{
    /* malloc(0) may return NULL, which would look like OOM; allocate at
     * least one element so n == 0 stays a valid, readable allocation. */
    uint32_t *a = malloc(n > 0 ? n * sizeof(uint32_t) : sizeof(uint32_t));
    if (a == NULL) {
        printf("out of memory\n");
        exit(1);
    }
    return a;
}

static uint32_t *make_unique(size_t n)
{
    uint32_t *a = alloc_u32(n);
    uint32_t v = (uint32_t)(xorshift64() & 0xFF);
    for (size_t i = 0; i < n; i++) {
        a[i] = v;
        v += (uint32_t)(1 + (xorshift64() % 97)); /* strictly increasing */
    }
    return a;
}

static uint32_t *make_dups(size_t n)
{
    uint32_t *a = alloc_u32(n);
    uint32_t v = (uint32_t)(xorshift64() & 0xFF);
    for (size_t i = 0; i < n; i++) {
        a[i] = v;
        if ((xorshift64() & 3) == 0) /* 1/4 of the time: duplicate run */
            v += 0;
        else
            v += (uint32_t)(1 + (xorshift64() % 97));
    }
    return a;
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    /* 1. edge sizes: every hit, plus misses around the edges and gaps */
    static const size_t edge_sizes[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9,
                                         15, 16, 17, 31, 32, 33,
                                         63, 64, 65, 100, 1000 };
    for (size_t s = 0; s < sizeof(edge_sizes) / sizeof(edge_sizes[0]);
         s++) {
        size_t n = edge_sizes[s];
        uint32_t *a = make_unique(n);
        for (size_t i = 0; i < n; i++)
            check_unique(a, n, a[i]); /* every hit */
        if (n > 0) {
            if (a[0] > 0)
                check_unique(a, n, a[0] - 1); /* below min */
            if (a[n - 1] < UINT32_MAX)
                check_unique(a, n, a[n - 1] + 1); /* above max */
            for (size_t i = 0; i + 1 < n; i++)
                if (a[i] + 1 < a[i + 1])
                    check_unique(a, n, a[i] + 1); /* a gap */
            check_unique(a, n, 0);
            check_unique(a, n, UINT32_MAX);
        } else {
            check_unique(a, 0, 0);
            check_unique(a, 0, 1);
            check_unique(a, 0, UINT32_MAX);
        }
        free(a);
    }
    printf("edge sizes: ok (%llu checks so far)\n", checks);

    /* 2. boundary values present in the array */
    {
        uint32_t b[] = { 0, 1, 2, 100, UINT32_MAX - 1, UINT32_MAX };
        size_t n = sizeof(b) / sizeof(b[0]);
        for (size_t i = 0; i < n; i++)
            check_unique(b, n, b[i]);
        check_unique(b, n, 3);
        check_unique(b, n, 99);
        check_unique(b, n, 101);
        check_unique(b, n, UINT32_MAX - 2);
    }
    printf("boundary values: ok (%llu checks so far)\n", checks);

    /* 3. duplicates: distinct values and gap values */
    for (size_t t = 0; t < 40; t++) {
        size_t n = 1 + (size_t)(xorshift64() % 500);
        uint32_t *a = make_dups(n);
        for (size_t i = 0; i < n; i += 1 + (size_t)(xorshift64() % 7))
            check_dups(a, n, a[i]);
        for (size_t i = 0; i < 20; i++)
            check_dups(a, n, (uint32_t)xorshift64());
        free(a);
    }
    printf("duplicates: ok (%llu checks so far)\n", checks);

    /* 4. bulk randomized differential on a larger unique array */
    {
        size_t n = 100000;
        uint32_t *a = make_unique(n);
        for (size_t i = 0; i < 200000; i++) {
            if ((xorshift64() & 1) == 0)
                check_unique(a, n,
                             a[xorshift64() % n]); /* hit */
            else
                check_unique(a, n,
                             (uint32_t)xorshift64()); /* likely miss */
        }
        free(a);
    }
    printf("bulk differential: ok (%llu checks so far)\n", checks);

    /* 5. iteration count: identical for every key at a given n */
    for (size_t n = 1; n <= 70; n++) {
        uint32_t *a = make_unique(n);
        unsigned want = expected_iters(n);
        for (size_t i = 0; i < n; i++) {
            unsigned it = 0;
            long r = bsearch_counted(a, n, a[i], &it);
            checks++;
            if (it != want || r != oracle_index(a, n, a[i]))
                fail("iters-hit", n, a[i], (long)it, (long)want);
            it = 0;
            r = bsearch_counted(a, n, (uint32_t)(a[i] + 1), &it);
            checks++;
            if (it != want)
                fail("iters-miss", n, a[i] + 1, (long)it,
                     (long)want);
            (void)r;
        }
        free(a);
    }
    printf("iteration counts: ok (%llu checks so far)\n", checks);

    if (mismatches != 0) {
        printf("FAIL: %llu mismatches\n", mismatches);
        return 1;
    }
    printf("correctness: %llu differential checks, 0 mismatches\n",
           checks);

    /* 6. benchmark: identical key sets, hits and misses, rdtsc cycles.
     * Two regimes: a 64 KiB array (L2-resident: measures the search
     * mechanism itself) and a 4 MiB array (measures the mechanism under
     * cache misses). Timing loops are pure; agreement with the oracle is
     * re-checked in a separate untimed pass so verification never
     * pollutes the cycle counts. */
    static const size_t bench_sizes[] = { 1 << 14, 1 << 20 };
    const size_t nkeys = 1000000;
    uint32_t *hits = malloc(nkeys * sizeof(uint32_t));
    uint32_t *miss = malloc(nkeys * sizeof(uint32_t));
    if (hits == NULL || miss == NULL) {
        printf("out of memory\n");
        return 1;
    }
    rng_state = 0xDEADBEEFCAFEBABEULL; /* fixed bench keys */
    for (size_t i = 0; i < nkeys; i++)
        hits[i] = (uint32_t)xorshift64();
    for (size_t i = 0; i < nkeys; i++)
        miss[i] = (uint32_t)xorshift64();

    uint64_t sink = 0;
    unsigned long long bench_checks = 0;
    unsigned long long bench_mismatch = 0;

    for (size_t b = 0; b < sizeof(bench_sizes) / sizeof(bench_sizes[0]);
         b++) {
        size_t bn = bench_sizes[b];
        uint32_t *ba = malloc(bn * sizeof(uint32_t));
        if (ba == NULL) {
            printf("out of memory\n");
            return 1;
        }
        for (size_t i = 0; i < bn; i++)
            ba[i] = (uint32_t)(i * 2); /* even values only */
        /* fold the raw keys into in-range hits and guaranteed misses */
        for (size_t i = 0; i < nkeys; i++)
            hits[i] = (uint32_t)((hits[i] % bn) * 2);
        for (size_t i = 0; i < nkeys; i++)
            miss[i] = (uint32_t)((miss[i] % bn) * 2 + 1); /* odd: absent */

        /* one warmup round, untimed */
        for (size_t i = 0; i < nkeys; i++)
            sink += (uint64_t)(bsearch_branchless(ba, bn, hits[i]) + 2);

        for (int round = 0; round < 3; round++) {
            uint64_t t0, t1;
            double per;

            t0 = rdtsc_begin();
            for (size_t i = 0; i < nkeys; i++)
                sink += (uint64_t)(bsearch_branchless(ba, bn,
                                                      hits[i]) +
                                   2);
            t1 = rdtsc_end();
            per = (double)(t1 - t0) / (double)nkeys;
            printf("n=%7zu round %d branchless hits  : %7.1f cycles/lookup\n",
                   bn, round, per);

            t0 = rdtsc_begin();
            for (size_t i = 0; i < nkeys; i++)
                sink += (uint64_t)(bsearch_branchless(ba, bn,
                                                      miss[i]) +
                                   2);
            t1 = rdtsc_end();
            per = (double)(t1 - t0) / (double)nkeys;
            printf("n=%7zu round %d branchless misses: %7.1f cycles/lookup\n",
                   bn, round, per);

            t0 = rdtsc_begin();
            for (size_t i = 0; i < nkeys; i++)
                sink += (uint64_t)(oracle_index(ba, bn, hits[i]) + 2);
            t1 = rdtsc_end();
            per = (double)(t1 - t0) / (double)nkeys;
            printf("n=%7zu round %d libc bsearch hits  : %7.1f cycles/lookup\n",
                   bn, round, per);

            t0 = rdtsc_begin();
            for (size_t i = 0; i < nkeys; i++)
                sink += (uint64_t)(oracle_index(ba, bn, miss[i]) + 2);
            t1 = rdtsc_end();
            per = (double)(t1 - t0) / (double)nkeys;
            printf("n=%7zu round %d libc bsearch misses: %7.1f cycles/lookup\n",
                   bn, round, per);
        }

        /* untimed agreement pass over the same key sets */
        for (size_t i = 0; i < nkeys; i++) {
            long r = bsearch_branchless(ba, bn, hits[i]);
            bench_checks++;
            if (r != oracle_index(ba, bn, hits[i]))
                bench_mismatch++;
            r = bsearch_branchless(ba, bn, miss[i]);
            bench_checks++;
            if (r != -1)
                bench_mismatch++;
        }
        free(ba);
        /* restore raw keys for the next size */
        rng_state = 0xDEADBEEFCAFEBABEULL;
        for (size_t i = 0; i < nkeys; i++)
            hits[i] = (uint32_t)xorshift64();
        for (size_t i = 0; i < nkeys; i++)
            miss[i] = (uint32_t)xorshift64();
    }

    printf("checksum sink: %llu (prevents dead-code elimination)\n",
           (unsigned long long)sink);
    printf("benchmark agreement: %llu timed lookups, %llu mismatches\n",
           bench_checks, bench_mismatch);
    printf("total differential checks: %llu, total mismatches: %llu\n",
           checks + bench_checks, mismatches + bench_mismatch);

    free(hits);
    free(miss);

    if (mismatches + bench_mismatch != 0)
        return 1;
    printf("ALL TESTS PASSED\n");
    return 0;
}
