/*
 * test_fma_mod: differential verification of fma_mod64 (fma_mod.h)
 * against the unsigned __int128 oracle
 *     ((unsigned __int128)a * b + c), truncated to 64 bits.
 *
 * Sections:
 *   [1] anchors: hand-checked values (0, identity, (2^64-1)^2 == 1).
 *   [2] exhaustive: all 2^32 pairs (a, b) with a, b < 2^16, each with
 *       8 fixed c values.
 *   [3] directed edge triples: all-zeros, all-ones, half-word
 *       boundaries, single-bit sweeps, and triples constructed to
 *       force the middle-column carries k1 and k2 of the header.
 *   [4] random: 10,000,000 fixed-seed splitmix64 64-bit triples.
 *   [5] throughput at -O2 over pre-generated triples.
 *
 * The carry tally in [3] and [4] is computed independently of the
 * header with unsigned __int128: col1 = al*bh + ah*bl + (al*bl >> 32)
 * as a 65-bit value; k1 = (p1 + p2 >= 2^64), k2 = (mid + (p0>>32) >=
 * 2^64).  It documents that the carry paths were genuinely exercised.
 *
 * Usage: ./test_fma_mod_o0 | ./test_fma_mod_o2 | ./test_fma_mod_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fma_mod.h"

/* Test oracle: exact (a * b + c) mod 2^64.  Only the oracle may use
 * unsigned __int128; fma_mod.h itself uses none. */
static uint64_t ref(uint64_t a, uint64_t b, uint64_t c)
{
    return (uint64_t)((unsigned __int128)a * b + c);
}

/* splitmix64, fixed seed: deterministic 64-bit stream.
 * Seed 0xF1A074F74A04D0D is arbitrary and documented. */
static uint64_t splitmix_state = UINT64_C(0xF1A074F74A04D0D);
static uint64_t splitmix_next(void)
{
    uint64_t z = (splitmix_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* FNV-1a 64 over a stream of words */
static uint64_t fnv_acc = UINT64_C(0xCBF29CE484222325);
static void fnv_add(uint64_t w)
{
    fnv_acc ^= w;
    fnv_acc *= UINT64_C(0x100000001B3);
}

static uint64_t total_cases;
static uint64_t total_mismatches;

/* Independent carry tally (unsigned __int128, no shared code with the
 * header's carry logic): how many triples set the middle-column
 * carries k1 = (p1+p2 wraps) and k2 = (mid + (p0>>32) wraps). */
static uint64_t tally_k1, tally_k2;
static void tally_carries(uint64_t a, uint64_t b)
{
    uint64_t al = (uint32_t)a, ah = a >> 32;
    uint64_t bl = (uint32_t)b, bh = b >> 32;
    unsigned __int128 p0 = (unsigned __int128)al * bl;
    unsigned __int128 p1 = (unsigned __int128)al * bh;
    unsigned __int128 p2 = (unsigned __int128)ah * bl;
    unsigned __int128 s1 = p1 + p2;
    if ((s1 >> 64) != 0)
        tally_k1++;
    unsigned __int128 s2 = (uint64_t)s1 + (p0 >> 32);
    if ((s2 >> 64) != 0)
        tally_k2++;
}

static void check_one(uint64_t a, uint64_t b, uint64_t c, int tally)
{
    uint64_t got = fma_mod64(a, b, c);
    uint64_t want = ref(a, b, c);
    if (got != want) {
        if (total_mismatches < 8)
            printf("  FAIL: a=%016llx b=%016llx c=%016llx got=%016llx want=%016llx\n",
                   (unsigned long long)a, (unsigned long long)b,
                   (unsigned long long)c, (unsigned long long)got,
                   (unsigned long long)want);
        total_mismatches++;
    }
    if (tally)
        tally_carries(a, b);
    fnv_add(got);
    total_cases++;
}

/* The 8 fixed c values used for the exhaustive 16-bit sweep. */
static const uint64_t C_SWEEP[8] = {
    UINT64_C(0x0000000000000000),
    UINT64_C(0x0000000000000001),
    UINT64_C(0x0000000000000002),
    UINT64_C(0x00000000FFFFFFFF),
    UINT64_C(0x0000000100000000),
    UINT64_C(0xFFFFFFFFFFFFFFFF),
    UINT64_C(0x8000000000000000),
    UINT64_C(0x123456789ABCDEF0),
};
#define N_CSWEEP 8

struct triple { uint64_t a, b, c; };

/* Directed edge triples.  The k1/k2 cases are constructed so the
 * header's middle-column carries are forced:
 *   k1 = 1: a = b = 2^64 - 1 gives p1 = p2 = (2^32-1)^2,
 *           p1 + p2 = 2^65 - 2^34 + 2, which wraps.
 *   k2 = 1: a = 0xFFFFFFFFFFFFFFFF, b = 0x00000002FFFFFFFF gives
 *           p1 + p2 = 2^64 - 1 (no wrap) and
 *           (p0 >> 32) = 2^32 - 2, so mid + (p0>>32) wraps.
 */
static const struct triple DIRECTED[] = {
    { UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000), UINT64_C(0xFFFFFFFFFFFFFFFF) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x0000000000000000), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000000000000) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000000000000) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000000000001) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000000000001), UINT64_C(0xFFFFFFFFFFFFFFFF) },
    { UINT64_C(0x0000000000000001), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x0000000000000001), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001) },
    /* k1 = 1 forced */
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x123456789ABCDEF0) },
    /* k2 = 1 forced (k1 = 0) */
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x00000002FFFFFFFF), UINT64_C(0x0000000000000000) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x00000002FFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF) },
    /* half-word boundaries */
    { UINT64_C(0xFFFFFFFF00000000), UINT64_C(0xFFFFFFFF00000000), UINT64_C(0x0000000000000000) },
    { UINT64_C(0xFFFFFFFF00000000), UINT64_C(0x00000000FFFFFFFF), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x00000000FFFFFFFF), UINT64_C(0xFFFFFFFF00000000), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x00000000FFFFFFFF), UINT64_C(0x00000000FFFFFFFF), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x8000000000000000), UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x8000000000000000), UINT64_C(0x0000000000000001), UINT64_C(0xFFFFFFFFFFFFFFFF) },
    { UINT64_C(0x0000000080000000), UINT64_C(0x0000000080000000), UINT64_C(0x0000000000000000) },
    { UINT64_C(0x0000000100000000), UINT64_C(0x0000000100000000), UINT64_C(0x0000000000000000) },
    { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0x0000000100000000), UINT64_C(0x0000000000000000) },
    /* +c wrap boundaries: (a*b) mod 2^64 near the top, c pushes over */
    { UINT64_C(0x0000000000000001), UINT64_C(0x0000000000000001), UINT64_C(0xFFFFFFFFFFFFFFFF) },
    { UINT64_C(0xFFFFFFFFFFFFFFFE), UINT64_C(0x0000000000000002), UINT64_C(0x0000000000000005) },
};
#define N_DIRECTED (sizeof(DIRECTED) / sizeof(DIRECTED[0]))

#define N_RANDOM 10000000u

int main(void)
{
    printf("fma_mod64 differential test, build %s\n", BUILD_NAME);

    /* [1/5] anchors with hand-checked values */
    printf("[1/5] anchors\n");
    struct { uint64_t a, b, c, want; } anchors[] = {
        /* (0 * b + c) mod 2^64 = c */
        { 0, 0, 0, 0 },
        { 0, UINT64_C(0x123456789ABCDEF0), UINT64_C(0x0FEDCBA987654321),
          UINT64_C(0x0FEDCBA987654321) },
        /* (a * 0 + c) mod 2^64 = c */
        { UINT64_C(0x123456789ABCDEF0), 0, UINT64_C(0x0FEDCBA987654321),
          UINT64_C(0x0FEDCBA987654321) },
        /* (2^64 - 1)^2 = 2^128 - 2^65 + 1 = 1 (mod 2^64) */
        { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF), 0, 1 },
        /* ... plus (2^64 - 1): 1 + (2^64 - 1) = 0 (mod 2^64) */
        { UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFF),
          UINT64_C(0xFFFFFFFFFFFFFFFF), 0 },
        { 1, 1, 0, 1 },
        { 2, 3, 4, 10 },
    };
    for (size_t i = 0; i < sizeof(anchors) / sizeof(anchors[0]); i++) {
        uint64_t got = fma_mod64(anchors[i].a, anchors[i].b, anchors[i].c);
        if (got != anchors[i].want) {
            printf("  FAIL anchor %zu: got %016llx want %016llx\n", i,
                   (unsigned long long)got, (unsigned long long)anchors[i].want);
            total_mismatches++;
        }
        fnv_add(got);
        total_cases++;
    }
    printf("  anchors checked: %llu, mismatches so far: %llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [2/5] exhaustive 16-bit (a, b) pairs x 8 c values */
    printf("[2/5] exhaustive 16-bit (a,b) pairs x %d c values\n", N_CSWEEP);
    for (uint64_t a = 0; a < 65536; a++) {
        for (uint64_t b = 0; b < 65536; b++) {
            for (int i = 0; i < N_CSWEEP; i++)
                check_one(a, b, C_SWEEP[i], 0);
        }
    }
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [3/5] directed edge triples, with independent carry tally */
    printf("[3/5] directed edge triples\n");
    tally_k1 = tally_k2 = 0;
    for (size_t i = 0; i < N_DIRECTED; i++)
        check_one(DIRECTED[i].a, DIRECTED[i].b, DIRECTED[i].c, 1);
    /* single-bit sweeps: vary one input at a time against fixed others */
    {
        const uint64_t A = UINT64_C(0x9E3779B97F4A7C15);
        const uint64_t B = UINT64_C(0xBF58476D1CE4E5B9);
        const uint64_t C = UINT64_C(0x94D049BB133111EB);
        for (int i = 0; i < 64; i++) {
            check_one(UINT64_C(1) << i, B, C, 1);
            check_one(A, UINT64_C(1) << i, C, 1);
            check_one(A, B, UINT64_C(1) << i, 1);
        }
    }
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);
    printf("  carry tally over directed triples: k1 events=%llu k2 events=%llu\n",
           (unsigned long long)tally_k1, (unsigned long long)tally_k2);
    if (tally_k1 == 0 || tally_k2 == 0) {
        printf("  FAIL: carry paths not exercised in directed set\n");
        total_mismatches++;
    }

    /* [4/5] random 64-bit triples, fixed seed, with carry tally */
    printf("[4/5] random 64-bit triples (splitmix64, seed 0xF1A074F74A04D0D)\n");
    tally_k1 = tally_k2 = 0;
    for (uint32_t i = 0; i < N_RANDOM; i++)
        check_one(splitmix_next(), splitmix_next(), splitmix_next(), 1);
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);
    printf("  carry tally over random triples: k1 events=%llu k2 events=%llu\n",
           (unsigned long long)tally_k1, (unsigned long long)tally_k2);

    /* [5/5] throughput: PRNG pre-generated, excluded from timing */
    printf("[5/5] throughput (PRNG pre-generated, excluded from timing)\n");
    {
        const size_t N = 2000000;
        uint64_t *ta = malloc(N * sizeof *ta);
        uint64_t *tb = malloc(N * sizeof *tb);
        uint64_t *tc = malloc(N * sizeof *tc);
        if (!ta || !tb || !tc) {
            printf("  FAIL: malloc\n");
            return 1;
        }
        for (size_t i = 0; i < N; i++) {
            ta[i] = splitmix_next();
            tb[i] = splitmix_next();
            tc[i] = splitmix_next();
        }
        double best = 1e30;
        for (int pass = 0; pass < 5; pass++) {
            volatile uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (size_t i = 0; i < N; i++)
                sink ^= fma_mod64(ta[i], tb[i], tc[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double ns = s / N * 1e9;
            if (ns < best)
                best = ns;
            printf("  throughput pass %d: %.3f ns/triple (%.3f M triples/s) sink=%llx\n",
                   pass, ns, 1000.0 / ns, (unsigned long long)sink);
        }
        printf("  throughput best of 5: %.3f ns/triple (%.3f M triples/s)\n",
               best, 1000.0 / best);
        free(ta);
        free(tb);
        free(tc);
    }

    printf("total verification cases: %llu\n", (unsigned long long)total_cases);
    printf("total mismatches: %llu\n", (unsigned long long)total_mismatches);
    printf("FNV-1a checksum of all outputs: 0x%016llx\n", (unsigned long long)fnv_acc);
    printf("RESULT: %s\n", total_mismatches == 0 ? "PASS" : "FAIL");
    return total_mismatches == 0 ? 0 : 1;
}
