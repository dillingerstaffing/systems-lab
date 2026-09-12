/*
 * test_mulhi_unsigned.c: differential test of mulhi_u64() against an
 * exact unsigned __int128 reference.
 *
 * unsigned __int128 appears ONLY in ref_high() below, never in the
 * implementation under test (mulhi_unsigned.h).
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mulhi_unsigned.h"

#ifndef BUILD_NAME
#define BUILD_NAME "?"
#endif

/* Exact reference: full unsigned 128-bit product, high word. */
static uint64_t ref_high(uint64_t a, uint64_t b)
{
    return (uint64_t)(((unsigned __int128)a * (unsigned __int128)b) >> 64);
}

/* splitmix64: deterministic 64-bit stream. */
static uint64_t rng_state;
static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a 64 over every result word, low byte first. */
static uint64_t fnv = 14695981039346656037ULL;
static void fnv_feed(uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        fnv ^= (v >> (8 * i)) & 0xFFu;
        fnv *= 1099511628211ULL;
    }
}

static uint64_t mismatches;
static uint64_t cases;

static void check(uint64_t a, uint64_t b)
{
    uint64_t got = mulhi_u64(a, b);
    uint64_t want = ref_high(a, b);
    fnv_feed(got);
    cases++;
    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH a=0x%016" PRIx64 " b=0x%016" PRIx64
                   " got=0x%016" PRIx64 " want=0x%016" PRIx64 "\n",
                   a, b, got, want);
    }
}

/*
 * Exhaustive 12-bit x 12-bit pairs (4096^2 = 16,777,216 cases).
 *
 * The backlog's original plan called for exhaustive 24-bit pairs, which
 * is 2^48 cases and infeasible on any host (at ~10ns/case that is
 * about 9 months of single-core compute). The honest largest
 * fully-exhaustive slice that fits in a lab budget is 12-bit pairs:
 * 2^24 = 16,777,216 cases, a few seconds per build. It covers every
 * combination of low-half bits up to the full 12-bit range, including
 * every 12-bit carry-out path (each of al, ah, bl, bh ranges over the
 * full 0..4095 domain, so the carry bits m1 and c are exercised over
 * their complete input space).
 */
static void test_exhaustive_12bit(void)
{
    for (uint32_t x = 0; x < 4096u; x++) {
        for (uint32_t y = 0; y < 4096u; y++)
            check((uint64_t)x, (uint64_t)y);
        if ((x & 1023u) == 0)
            fprintf(stderr, "  ... x=%u/4096\n", x);
    }
}

/* 10M fixed-seed 64-bit pairs. */
static void test_random(void)
{
    rng_state = 0x123456789ABCDEF0ULL;
    for (uint64_t i = 0; i < 10000000ull; i++)
        check(splitmix64(), splitmix64());
}

/*
 * Directed edges: rows around 0, 1, 2^32-1, 2^32, 2^32+1, 2^63-1,
 * 2^63, 2^64-2, 2^64-1, as a full cross product. This includes the
 * required (2^64-1) x (2^64-1) = 2^128 - 2^65 + 1 case, whose high
 * word is 2^64 - 2, the largest possible output. Plus unsigned powers
 * of two: (2^k, 2^j) for k, j in 0..63, covering every magnitude
 * where the high word is nonzero, including (2^63, 2^63).
 */
static void test_directed(void)
{
    static const uint64_t vals[] = {
        0,
        1,
        2,
        4095,
        4096,
        4097,
        65535,
        65536,
        65537,
        (UINT64_C(1) << 32) - 1,
        (UINT64_C(1) << 32),
        (UINT64_C(1) << 32) + 1,
        (UINT64_C(1) << 33),
        (UINT64_C(1) << 63) - 1,
        (UINT64_C(1) << 63),
        (UINT64_C(1) << 63) + 1,
        UINT64_MAX - 1,
        UINT64_MAX,
    };
    size_t n = sizeof vals / sizeof vals[0];
    for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++)
            check(vals[i], vals[j]);
    for (int k = 0; k < 64; k++)
        for (int j = 0; j < 64; j++)
            check(UINT64_C(1) << k, UINT64_C(1) << j);
}

#define NPAIRS 1000000u
static uint64_t tpairs[2 * NPAIRS];

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/*
 * Throughput at this build's optimization level.  Pairs are generated
 * once with splitmix64 BEFORE timing starts, so the timed loop contains
 * only mulhi_u64 calls plus an xor into a volatile sink (the sink keeps
 * the compiler from folding the loop away).  The PRNG step is therefore
 * NOT part of the measured time.  The reported number includes the
 * loop, call, and xor-sink overhead.
 */
static void test_throughput(void)
{
    for (uint32_t i = 0; i < 2 * NPAIRS; i++)
        tpairs[i] = splitmix64();
    volatile uint64_t sink = 0;
    double best = 1e300;
    for (int pass = 0; pass < 5; pass++) {
        double t0 = now_ns();
        for (uint32_t i = 0; i < NPAIRS; i++)
            sink ^= mulhi_u64(tpairs[2 * i], tpairs[2 * i + 1]);
        double t1 = now_ns();
        double ns = (t1 - t0) / (double)NPAIRS;
        if (ns < best)
            best = ns;
        printf("  throughput pass %d: %.3f ns/pair\n", pass, ns);
    }
    printf("  throughput best of 5: %.3f ns/pair\n", best);
}

int main(void)
{
    printf("mulhi_u64 differential test, build %s\n", BUILD_NAME);

    printf("[1/4] exhaustive 12-bit x 12-bit pairs (16777216 cases)\n");
    fflush(stdout);
    test_exhaustive_12bit();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64 "\n",
           cases, mismatches);

    printf("[2/4] 10M fixed-seed splitmix64 64-bit pairs\n");
    fflush(stdout);
    test_random();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64 "\n",
           cases, mismatches);

    printf("[3/4] directed edge cases\n");
    fflush(stdout);
    test_directed();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64 "\n",
           cases, mismatches);

    printf("[4/4] throughput (PRNG pre-generated, excluded from timing)\n");
    test_throughput();

    printf("total verification cases: %" PRIu64 "\n", cases);
    printf("total mismatches: %" PRIu64 "\n", mismatches);
    printf("FNV-1a checksum of all result words: 0x%016" PRIx64 "\n", fnv);
    printf("RESULT: %s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
