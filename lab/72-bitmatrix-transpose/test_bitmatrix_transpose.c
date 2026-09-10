/*
 * test_bitmatrix_transpose.c: differential test of bitmatrix_transpose64()
 * against an independent naive per-bit-loop reference.
 *
 * The reference moves bit (r, c) to (c, r) one bit at a time and shares
 * no code with the implementation under test (bitmatrix_transpose.h).
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bitmatrix_transpose.h"

#ifndef BUILD_NAME
#define BUILD_NAME "?"
#endif

/* Independent reference: per-bit loop, bit (r,c) -> bit (c,r). */
static uint64_t ref_transpose(uint64_t x)
{
    uint64_t out = 0;
    for (unsigned r = 0; r < 8; r++) {
        for (unsigned c = 0; c < 8; c++) {
            if ((x >> (8u * r + c)) & 1u)
                out |= (uint64_t)1 << (8u * c + r);
        }
    }
    return out;
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
static uint64_t involution_failures;

static void check(uint64_t x)
{
    uint64_t got = bitmatrix_transpose64(x);
    uint64_t want = ref_transpose(x);
    uint64_t back = bitmatrix_transpose64(got);
    fnv_feed(got);
    fnv_feed(back);
    cases++;
    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH x=%016" PRIx64 " got=%016" PRIx64
                   " want=%016" PRIx64 "\n",
                   x, got, want);
    }
    if (back != x) {
        involution_failures++;
        if (involution_failures < 10)
            printf("INVOLUTION-FAIL x=%016" PRIx64 " back=%016" PRIx64 "\n",
                   x, back);
    }
}

/* All 2^16 inputs. */
static void test_exhaustive_16bit(void)
{
    for (uint32_t x = 0; x < 65536u; x++)
        check((uint64_t)x);
}

/* 10M fixed-seed 64-bit values. */
static void test_random(void)
{
    rng_state = 0x123456789ABCDEF0ULL;
    for (uint64_t i = 0; i < 10000000ull; i++)
        check(splitmix64());
}

/* Directed edges: zero, all-ones, single bits at each of the 64
 * positions, alternating row/column/checkerboard patterns, and the
 * 8 single-row / 8 single-column masks (a full row must become a full
 * column and vice versa). */
static void test_directed(void)
{
    check(0ULL);
    check(0xFFFFFFFFFFFFFFFFULL);
    for (int i = 0; i < 64; i++)
        check(1ULL << i);
    check(0xAAAAAAAAAAAAAAAAULL);
    check(0x5555555555555555ULL);
    check(0xFF00FF00FF00FF00ULL);
    check(0x00FF00FF00FF00FFULL);
    for (int r = 0; r < 8; r++)
        check(0xFFULL << (8 * r));       /* single full row */
    for (int c = 0; c < 8; c++) {
        uint64_t col = 0;
        for (int r = 0; r < 8; r++)
            col |= (uint64_t)1 << (8 * r + c);
        check(col);                     /* single full column */
    }
}

#define NVALS 1000000u
static uint64_t tvals[NVALS];

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/*
 * Throughput at this build's optimization level.  Values are generated
 * once with splitmix64 BEFORE timing starts, so the timed loop contains
 * only bitmatrix_transpose64 calls plus an xor into a volatile sink (the
 * sink keeps the compiler from folding the loop away).  The PRNG step is
 * therefore NOT part of the measured time.
 */
static void test_throughput(void)
{
    for (uint32_t i = 0; i < NVALS; i++)
        tvals[i] = splitmix64();
    volatile uint64_t sink = 0;
    double best = 1e300;
    for (int pass = 0; pass < 5; pass++) {
        double t0 = now_ns();
        for (uint32_t i = 0; i < NVALS; i++)
            sink ^= bitmatrix_transpose64(tvals[i]);
        double t1 = now_ns();
        double ns = (t1 - t0) / (double)NVALS;
        if (ns < best)
            best = ns;
        printf("  throughput pass %d: %.3f ns/value (%.3f M values/s)\n",
               pass, ns, 1000.0 / ns);
    }
    printf("  throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
           best, 1000.0 / best);
}

int main(void)
{
    printf("bitmatrix_transpose64 differential test, build %s\n", BUILD_NAME);

    printf("[1/4] exhaustive 16-bit inputs (65536 cases)\n");
    fflush(stdout);
    test_exhaustive_16bit();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64
           " involution_failures=%" PRIu64 "\n",
           cases, mismatches, involution_failures);

    printf("[2/4] 10M fixed-seed splitmix64 64-bit values\n");
    fflush(stdout);
    test_random();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64
           " involution_failures=%" PRIu64 "\n",
           cases, mismatches, involution_failures);

    printf("[3/4] directed edge cases\n");
    fflush(stdout);
    test_directed();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64
           " involution_failures=%" PRIu64 "\n",
           cases, mismatches, involution_failures);

    printf("[4/4] throughput (PRNG pre-generated, excluded from timing)\n");
    test_throughput();

    printf("total verification cases: %" PRIu64 "\n", cases);
    printf("total mismatches: %" PRIu64 "\n", mismatches);
    printf("total involution failures: %" PRIu64 "\n", involution_failures);
    printf("FNV-1a checksum of all result words: 0x%016" PRIx64 "\n", fnv);
    printf("RESULT: %s\n",
           (mismatches == 0 && involution_failures == 0) ? "PASS" : "FAIL");
    return (mismatches == 0 && involution_failures == 0) ? 0 : 1;
}
