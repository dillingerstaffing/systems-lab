/*
 * test_mul_high.c: differential test of mul_high64() against an exact
 * unsigned __int128 reference.
 *
 * __int128 appears ONLY in ref_high() below, never in the implementation
 * under test (mul_high.h).
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mul_high.h"

#ifndef BUILD_NAME
#define BUILD_NAME "?"
#endif

/* Exact reference: full 128-bit product, high word. */
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
    uint64_t got = mul_high64(a, b);
    uint64_t want = ref_high(a, b);
    fnv_feed(got);
    cases++;
    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH a=%016" PRIx64 " b=%016" PRIx64
                   " got=%016" PRIx64 " want=%016" PRIx64 "\n",
                   a, b, got, want);
    }
}

/* All 2^32 pairs of 16-bit inputs. */
static void test_exhaustive_16bit(void)
{
    for (uint32_t x = 0; x < 65536u; x++) {
        for (uint32_t y = 0; y < 65536u; y++)
            check((uint64_t)x, (uint64_t)y);
        if ((x & 8191u) == 0)
            fprintf(stderr, "  ... x=%u/65536\n", x);
    }
}

/* 10M fixed-seed 64-bit pairs. */
static void test_random(void)
{
    rng_state = 0x123456789ABCDEF0ULL;
    for (uint64_t i = 0; i < 10000000ull; i++)
        check(splitmix64(), splitmix64());
}

/* Directed edges: zeros, maxima, powers of two, half-word boundaries,
 * and values chosen to force each reachable carry state of the middle
 * sum: c1=1 via (UINT64_MAX, UINT64_MAX); c2=1 via
 * (0x80000000FFFFFFFF, 0x80000001FFFFFFFF); c1=c2=1 is impossible
 * (it would need p0>>32 >= 2^34 - 2, but p0>>32 < 2^32). */
static void test_directed(void)
{
    static const uint64_t vals[] = {
        0ULL,
        1ULL,
        2ULL,
        0x00000000FFFFFFFFULL,
        0xFFFFFFFF00000000ULL,
        0x00000001FFFFFFFFULL,
        0xFFFFFFFF00000001ULL,
        0x0000000100000000ULL,
        0x7FFFFFFF7FFFFFFFULL,
        0x8000000080000000ULL,
        0x8000000000000000ULL,
        0x80000000FFFFFFFFULL,
        0x80000001FFFFFFFFULL,
        0xAAAAAAAAAAAAAAAAULL,
        0x5555555555555555ULL,
        0xFFFFFFFFFFFFFFFEULL,
        0xFFFFFFFFFFFFFFFFULL,
    };
    size_t n = sizeof vals / sizeof vals[0];
    for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++)
            check(vals[i], vals[j]);
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++)
            check(1ULL << i, 1ULL << j);
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
 * only mul_high64 calls plus an xor into a volatile sink (the sink keeps
 * the compiler from folding the loop away).  The PRNG step is therefore
 * NOT part of the measured time.
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
            sink ^= mul_high64(tpairs[2 * i], tpairs[2 * i + 1]);
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
    printf("mul_high64 differential test, build %s\n", BUILD_NAME);

    printf("[1/4] exhaustive 16-bit x 16-bit pairs (4294967296 cases)\n");
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
