/*
 * test_mulhi_signed.c: differential test of mulhi_s64() against an exact
 * signed __int128 reference.
 *
 * signed __int128 appears ONLY in ref_high() below, never in the
 * implementation under test (mulhi_signed.h).
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mulhi_signed.h"

#ifndef BUILD_NAME
#define BUILD_NAME "?"
#endif

/* Exact reference: full signed 128-bit product, high word. */
static int64_t ref_high(int64_t a, int64_t b)
{
    return (int64_t)(((signed __int128)a * (signed __int128)b) >> 64);
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

static void check(int64_t a, int64_t b)
{
    int64_t got = mulhi_s64(a, b);
    int64_t want = ref_high(a, b);
    fnv_feed((uint64_t)got);
    cases++;
    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH a=%" PRId64 " (0x%016" PRIx64 ")"
                   " b=%" PRId64 " (0x%016" PRIx64 ")"
                   " got=%" PRId64 " want=%" PRId64 "\n",
                   a, (uint64_t)a, b, (uint64_t)b, got, want);
    }
}

/*
 * All 2^32 pairs of 16-bit inputs, sign-extended to int64.  This covers
 * every combination of operand signs, so every sign-correction path
 * (neither, a only, b only, both negative) is exercised over the full
 * 16-bit range, and the high-half sign extension (a negative 16-bit
 * value has high half 0xFFFFFFFF) is exercised in every case.
 */
static void test_exhaustive_16bit(void)
{
    for (uint32_t x = 0; x < 65536u; x++) {
        int64_t a = (int64_t)(int16_t)x;
        for (uint32_t y = 0; y < 65536u; y++)
            check(a, (int64_t)(int16_t)y);
        if ((x & 8191u) == 0)
            fprintf(stderr, "  ... x=%u/65536\n", x);
    }
}

/* 10M fixed-seed 64-bit pairs (bit patterns read as signed). */
static void test_random(void)
{
    rng_state = 0x123456789ABCDEF0ULL;
    for (uint64_t i = 0; i < 10000000ull; i++)
        check((int64_t)splitmix64(), (int64_t)splitmix64());
}

/*
 * Directed edges: sign-boundary rows around 0, +-1, +-2^16, +-2^32,
 * and the int64 extremes, as a full cross product.  This includes the
 * required cases INT64_MIN x INT64_MIN, INT64_MIN x -1,
 * INT64_MIN x INT64_MAX, and (-1) x (-1).  Plus signed powers of two:
 * for k, j in 0..62, (2^k, 2^j), (-2^k, -2^j), (-2^k, 2^j), covering
 * every sign combination at every magnitude where the high word is
 * nonzero.
 */
static void test_directed(void)
{
    static const int64_t vals[] = {
        INT64_MIN,
        INT64_MIN + 1,
        -(INT64_C(1) << 33),
        -(INT64_C(1) << 32) - 1,
        -(INT64_C(1) << 32),
        -(INT64_C(1) << 32) + 1,
        -65537,
        -65536,
        -65535,
        -2,
        -1,
        0,
        1,
        2,
        65535,
        65536,
        65537,
        (INT64_C(1) << 32) - 1,
        (INT64_C(1) << 32),
        (INT64_C(1) << 32) + 1,
        (INT64_C(1) << 33),
        INT64_MAX - 1,
        INT64_MAX,
    };
    size_t n = sizeof vals / sizeof vals[0];
    for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++)
            check(vals[i], vals[j]);
    for (int k = 0; k < 63; k++) {
        for (int j = 0; j < 63; j++) {
            int64_t p = INT64_C(1) << k;
            int64_t q = INT64_C(1) << j;
            check(p, q);
            check(-p, -q);
            check(-p, q);
        }
    }
}

#define NPAIRS 1000000u
static int64_t tpairs[2 * NPAIRS];

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/*
 * Throughput at this build's optimization level.  Pairs are generated
 * once with splitmix64 BEFORE timing starts, so the timed loop contains
 * only mulhi_s64 calls plus an xor into a volatile sink (the sink keeps
 * the compiler from folding the loop away).  The PRNG step is therefore
 * NOT part of the measured time.
 */
static void test_throughput(void)
{
    for (uint32_t i = 0; i < 2 * NPAIRS; i++)
        tpairs[i] = (int64_t)splitmix64();
    volatile uint64_t sink = 0;
    double best = 1e300;
    for (int pass = 0; pass < 5; pass++) {
        double t0 = now_ns();
        for (uint32_t i = 0; i < NPAIRS; i++)
            sink ^= (uint64_t)mulhi_s64(tpairs[2 * i], tpairs[2 * i + 1]);
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
    printf("mulhi_s64 differential test, build %s\n", BUILD_NAME);

    printf("[1/4] exhaustive sign-extended 16-bit x 16-bit pairs "
           "(4294967296 cases)\n");
    fflush(stdout);
    test_exhaustive_16bit();
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
