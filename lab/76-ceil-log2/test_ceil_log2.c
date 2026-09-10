/*
 * test_ceil_log2: differential verification of ceil_log2_64
 * (ceil_log2.h) against an independent exact loop reference.
 *
 * Sections:
 *   [1] anchors: hand-checked values, including the x = 0
 *       contract row (returns 0).
 *   [2] exhaustive: every 16-bit input (all 65,536 words < 2^16).
 *   [3] boundary: 2^k, 2^k - 1, 2^k + 1 for k = 0..63
 *       (all fit in 64 bits; the k = 0, 2^k - 1 = 0 case is the
 *       contract row, checked separately).
 *   [4] random: 10,000,000 fixed-seed splitmix64 64-bit words.
 *   [5] throughput at -O2 over 100M timed values.
 *
 * Usage: ./test_ceil_log2_o0 | ./test_ceil_log2_o2
 *        | ./test_ceil_log2_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ceil_log2.h"

/* Independent exact reference: smallest n >= 0 with 2^n >= x,
 * found by a plain doubling loop.  p is widened before it can
 * overflow: once p > UINT64_MAX/2 it becomes UINT64_MAX, which is
 * >= x for every finite x, so the loop still terminates with the
 * exact n.  No bit tricks, no relation to the implementation. */
static unsigned ref_ceil(uint64_t x)
{
    if (x <= 1)
        return 0;
    unsigned n = 0;
    uint64_t p = 1;
    while (p < x) {
        p = (p <= UINT64_C(0x7FFFFFFFFFFFFFFF)) ? p * 2 : UINT64_MAX;
        n++;
    }
    return n;
}

/* splitmix64, fixed seed: deterministic 64-bit stream.
 * Seed 0x123456789ABCDEF0 is the established lab convention. */
static uint64_t splitmix_state = UINT64_C(0x123456789ABCDEF0);
static uint64_t splitmix_next(void)
{
    uint64_t z = (splitmix_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* FNV-1a 64 over the stream of output counts */
static uint64_t fnv_acc = UINT64_C(0xCBF29CE484222325);
static void fnv_add(uint64_t w)
{
    fnv_acc ^= w;
    fnv_acc *= UINT64_C(0x100000001B3);
}

static uint64_t total_cases;
static uint64_t total_mismatches;

static void check_one(uint64_t x)
{
    unsigned got = ceil_log2_64(x);
    unsigned want = ref_ceil(x);
    if (got != want) {
        if (total_mismatches < 8)
            printf("  FAIL: x=%016llx got=%u want=%u\n",
                   (unsigned long long)x, got, want);
        total_mismatches++;
    }
    fnv_add(got);
    total_cases++;
}

/* Contract row: x = 0 returns 0 by contract, outside the
 * differential domain (the loop reference defines it as 0 too,
 * but the point under test is the documented contract). */
static void check_contract_zero(void)
{
    unsigned got = ceil_log2_64(0);
    if (got != 0) {
        printf("  FAIL: x=0 contract: got=%u want=0\n", got);
        total_mismatches++;
    }
    fnv_add(got);
    total_cases++;
}

#define N_RANDOM 10000000u

int main(void)
{
    printf("ceil_log2_64 differential test, build %s\n", BUILD_NAME);

    /* [1/5] anchors with hand-checked values */
    printf("[1/5] anchors\n");
    check_contract_zero();              /* x = 0 contract row */
    struct { uint64_t x; unsigned want; } anchors[] = {
        { UINT64_C(0x0000000000000001), 0 },   /* 2^0 */
        { UINT64_C(0x0000000000000002), 1 },   /* 2^1 */
        { UINT64_C(0x0000000000000003), 2 },
        { UINT64_C(0x0000000000000004), 2 },   /* 2^2 */
        { UINT64_C(0x0000000000000005), 3 },
        { UINT64_C(0x0000000000000007), 3 },
        { UINT64_C(0x0000000000000008), 3 },   /* 2^3 */
        { UINT64_C(0x0000000000000009), 4 },
        { UINT64_C(0x0000000100000000), 32 },  /* 2^32 */
        { UINT64_C(0x00000000FFFFFFFF), 32 }, /* 2^32 - 1 */
        { UINT64_C(0x0000000100000001), 33 }, /* 2^32 + 1 */
        { UINT64_C(0x8000000000000000), 63 },  /* 2^63 */
        { UINT64_C(0x7FFFFFFFFFFFFFFF), 63 },  /* 2^63 - 1 */
        { UINT64_C(0x8000000000000001), 64 },  /* 2^63 + 1 */
        { UINT64_C(0xFFFFFFFFFFFFFFFF), 64 },  /* 2^64 - 1 */
    };
    for (size_t i = 0; i < sizeof(anchors) / sizeof(anchors[0]); i++) {
        unsigned got = ceil_log2_64(anchors[i].x);
        if (got != anchors[i].want) {
            printf("  FAIL anchor %zu: got %u want %u\n", i,
                   got, anchors[i].want);
            total_mismatches++;
        }
        fnv_add(got);
        total_cases++;
    }
    printf("  anchors checked: %llu, mismatches so far: %llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [2/5] exhaustive 16-bit inputs */
    printf("[2/5] exhaustive 16-bit inputs\n");
    for (uint64_t x = 0; x < 65536; x++)
        check_one(x);
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [3/5] boundary rows: 2^k, 2^k - 1, 2^k + 1 for k = 0..63.
     * All three fit in 64 bits for every k (no overflow at
     * k = 63).  The k = 0, 2^k - 1 = 0 case is the contract row. */
    printf("[3/5] boundary rows 2^k, 2^k-1, 2^k+1, k = 0..63\n");
    for (unsigned k = 0; k < 64; k++) {
        uint64_t p2 = UINT64_C(1) << k;
        check_one(p2);          /* 2^k */
        if (p2 == 1)
            check_contract_zero();  /* 2^0 - 1 = 0: contract row */
        else
            check_one(p2 - 1);  /* 2^k - 1 */
        check_one(p2 + 1);      /* 2^k + 1 */
    }
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [4/5] random 64-bit words, fixed seed */
    printf("[4/5] random 64-bit words (splitmix64, seed 0x123456789ABCDEF0)\n");
    for (uint32_t i = 0; i < N_RANDOM; i++)
        check_one(splitmix_next());
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [5/5] throughput: 100M timed values at -O2.
     * PRNG pre-generated and excluded from timing; a 2M-word
     * buffer is reused for 50 passes, so the timed input stream
     * is 100,000,000 values. */
    printf("[5/5] throughput (PRNG pre-generated, excluded from timing)\n");
    {
        const size_t N = 2000000;
        const int PASSES = 50;          /* N * PASSES = 100M values */
        uint64_t *t = malloc(N * sizeof *t);
        if (!t) {
            printf("  FAIL: malloc\n");
            return 1;
        }
        for (size_t i = 0; i < N; i++)
            t[i] = splitmix_next();
        double best = 1e30;
        for (int pass = 0; pass < PASSES; pass++) {
            volatile uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (size_t i = 0; i < N; i++)
                sink += ceil_log2_64(t[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double ns = s / N * 1e9;
            if (ns < best)
                best = ns;
        }
        printf("  throughput best of %d passes over %zu pre-generated words\n",
               PASSES, N);
        printf("  (%llu total timed values): %.3f ns/value (%.3f M values/s)\n",
               (unsigned long long)N * (unsigned long long)PASSES,
               best, 1000.0 / best);
        /* Honesty check: same loop with the PRNG in the timed loop. */
        {
            splitmix_state = UINT64_C(0x123456789ABCDEF0);
            volatile uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (size_t i = 0; i < N; i++)
                sink += ceil_log2_64(splitmix_next());
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            printf("  with PRNG in the timed loop: %.3f ns/value (sink=%llx)\n",
                   s / N * 1e9, (unsigned long long)sink);
        }
        free(t);
    }

    printf("total verification cases: %llu\n", (unsigned long long)total_cases);
    printf("total mismatches: %llu\n", (unsigned long long)total_mismatches);
    printf("FNV-1a checksum of all outputs: 0x%016llx\n", (unsigned long long)fnv_acc);
    printf("RESULT: %s\n", total_mismatches == 0 ? "PASS" : "FAIL");
    return total_mismatches == 0 ? 0 : 1;
}
