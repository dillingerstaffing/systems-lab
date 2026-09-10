/*
 * test_leading_ones: differential verification of clo64 (leading_ones.h)
 * against a naive bit-by-bit reference scan.
 *
 * Sections:
 *   [1] anchors: hand-checked values, including the zero-input
 *       contract (clo(0xFFFFFFFFFFFFFFFF) = 64) and clo(0) = 0.
 *   [2] exhaustive: every 16-bit input (all 65,536 words < 2^16).
 *   [3] directed: for every run length k = 0..64, the word whose
 *       top k bits are 1 and the rest are 0 (covers every possible
 *       leading-ones count exactly), plus alternating and
 *       boundary patterns.
 *   [4] random: 10,000,000 fixed-seed splitmix64 64-bit words.
 *   [5] throughput at -O2 over pre-generated words.
 *
 * Usage: ./test_leading_ones_o0 | ./test_leading_ones_o2
 *        | ./test_leading_ones_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "leading_ones.h"

/* Naive reference: count consecutive 1 bits from the MSB down. */
static unsigned ref_clo(uint64_t x)
{
    unsigned n = 0;
    for (int i = 63; i >= 0 && ((x >> i) & 1u); i--)
        n++;
    return n;
}

/* splitmix64, fixed seed: deterministic 64-bit stream.
 * Seed 0xC10DCAFE12345678 is arbitrary and documented. */
static uint64_t splitmix_state = UINT64_C(0xC10DCAFE12345678);
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
    unsigned got = clo64(x);
    unsigned want = ref_clo(x);
    if (got != want) {
        if (total_mismatches < 8)
            printf("  FAIL: x=%016llx got=%u want=%u\n",
                   (unsigned long long)x, got, want);
        total_mismatches++;
    }
    fnv_add(got);
    total_cases++;
}

#define N_RANDOM 10000000u

int main(void)
{
    printf("clo64 differential test, build %s\n", BUILD_NAME);

    /* [1/5] anchors with hand-checked values */
    printf("[1/5] anchors\n");
    struct { uint64_t x; unsigned want; } anchors[] = {
        { UINT64_C(0x0000000000000000), 0 },   /* no leading ones */
        { UINT64_C(0x0000000000000001), 0 },   /* MSB clear */
        { UINT64_C(0x7FFFFFFFFFFFFFFF), 0 },  /* MSB clear, rest set */
        { UINT64_C(0x8000000000000000), 1 },   /* only the top bit */
        { UINT64_C(0xC000000000000000), 2 },
        { UINT64_C(0xE000000000000000), 3 },
        { UINT64_C(0xFF00000000000000), 8 },
        { UINT64_C(0xFFFFFFFF00000000), 32 },
        { UINT64_C(0xFFFFFFFFFFFFFFFE), 63 },
        /* zero input of the scan (~x = 0): contract returns 64 */
        { UINT64_C(0xFFFFFFFFFFFFFFFF), 64 },
    };
    for (size_t i = 0; i < sizeof(anchors) / sizeof(anchors[0]); i++) {
        unsigned got = clo64(anchors[i].x);
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

    /* [3/5] directed: every run length 0..64 as an exact witness,
     * plus alternating and boundary patterns. */
    printf("[3/5] directed edge words\n");
    for (unsigned k = 0; k <= 64; k++) {
        uint64_t x = (k == 0) ? UINT64_C(0)
                              : (k == 64) ? UINT64_C(0xFFFFFFFFFFFFFFFF)
                                           : (~UINT64_C(0) << (64 - k));
        check_one(x);
    }
    {
        const uint64_t PATS[] = {
            UINT64_C(0xAAAAAAAAAAAAAAAA),  /* 1010... : clo = 1 */
            UINT64_C(0x5555555555555555),  /* 0101... : clo = 0 */
            UINT64_C(0xF0F0F0F0F0F0F0F0),  /* clo = 4 */
            UINT64_C(0x0F0F0F0F0F0F0F0F),  /* clo = 0 */
            UINT64_C(0xFFFEFFFFFFFFFFFF),  /* clo = 15 */
            UINT64_C(0x7FFFFFFFFFFFFFFF),  /* clo = 0 */
            UINT64_C(0x8000000000000001),  /* clo = 1 */
            UINT64_C(0x0000FFFFFFFFFFFF),  /* clo = 0 */
        };
        for (size_t i = 0; i < sizeof(PATS) / sizeof(PATS[0]); i++)
            check_one(PATS[i]);
    }
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [4/5] random 64-bit words, fixed seed */
    printf("[4/5] random 64-bit words (splitmix64, seed 0xC10DCAFE12345678)\n");
    for (uint32_t i = 0; i < N_RANDOM; i++)
        check_one(splitmix_next());
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [5/5] throughput: PRNG pre-generated, excluded from timing */
    printf("[5/5] throughput (PRNG pre-generated, excluded from timing)\n");
    {
        const size_t N = 2000000;
        uint64_t *t = malloc(N * sizeof *t);
        if (!t) {
            printf("  FAIL: malloc\n");
            return 1;
        }
        for (size_t i = 0; i < N; i++)
            t[i] = splitmix_next();
        double best = 1e30;
        for (int pass = 0; pass < 5; pass++) {
            volatile uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (size_t i = 0; i < N; i++)
                sink += clo64(t[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double ns = s / N * 1e9;
            if (ns < best)
                best = ns;
            printf("  throughput pass %d: %.3f ns/value (%.3f M values/s) sink=%llx\n",
                   pass, ns, 1000.0 / ns, (unsigned long long)sink);
        }
        printf("  throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
               best, 1000.0 / best);
        /* Honesty check: same loop with the PRNG in the timed loop. */
        {
            splitmix_state = UINT64_C(0xC10DCAFE12345678);
            volatile uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (size_t i = 0; i < N; i++)
                sink += clo64(splitmix_next());
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
