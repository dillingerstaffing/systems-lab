/*
 * test_mulhi_u16.c: differential test of mulhi_u16() against the exact
 * 32-bit native product, shifted right 16.
 *
 * The native multiply appears ONLY in ref_high() below, the oracle.
 * The implementation under test (mulhi_u16.h) uses shifts and adds only.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mulhi_u16.h"

#ifndef BUILD_NAME
#define BUILD_NAME "?"
#endif

/* Exact reference: full 32-bit product, high 16 bits. Oracle only. */
static uint16_t ref_high(uint16_t a, uint16_t b)
{
    return (uint16_t)(((uint32_t)a * b) >> 16);
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
static void fnv_feed(uint16_t v)
{
    for (int i = 0; i < 2; i++) {
        fnv ^= (uint64_t)((v >> (8 * i)) & 0xFFu);
        fnv *= 1099511628211ULL;
    }
}

static uint64_t mismatches;
static uint64_t cases;

static void check(uint16_t a, uint16_t b)
{
    uint16_t got = mulhi_u16(a, b);
    uint16_t want = ref_high(a, b);
    fnv_feed(got);
    cases++;
    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH a=%u (0x%04x) b=%u (0x%04x)"
                   " got=%u (0x%04x) want=%u (0x%04x)\n",
                   a, a, b, b, got, got, want, want);
    }
}

/*
 * All 2^32 pairs of 16-bit inputs. This is the complete input space,
 * so every multiplier bit pattern (all 65,536 values of b, hence all
 * add/no-add paths) and every multiplicand value is exercised
 * exhaustively.
 */
static void test_exhaustive(void)
{
    for (uint32_t x = 0; x < 65536u; x++) {
        for (uint32_t y = 0; y < 65536u; y++)
            check((uint16_t)x, (uint16_t)y);
        if ((x & 8191u) == 0)
            fprintf(stderr, "  ... x=%u/65536\n", x);
    }
}

/*
 * Directed edges: boundary rows around 0, 1, 2, 0x7FFF/0x8000,
 * 0xFFFE/0xFFFF, and powers of two, as a full cross product. The
 * exhaustive pass covers all of these too; this pass runs first in
 * development order and pins the extremes explicitly.
 */
static void test_directed(void)
{
    static const uint16_t vals[] = {
        0, 1, 2, 3,
        0x7FFE, 0x7FFF, 0x8000, 0x8001,
        0xFFFD, 0xFFFE, 0xFFFF,
    };
    size_t n = sizeof vals / sizeof vals[0];
    for (size_t i = 0; i < n; i++)
        for (size_t j = 0; j < n; j++)
            check(vals[i], vals[j]);
    for (int k = 0; k < 16; k++)
        for (int j = 0; j < 16; j++)
            check((uint16_t)(1u << k), (uint16_t)(1u << j));
}

#define NPAIRS 1000000u
static uint16_t tpairs[2 * NPAIRS];

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/*
 * Throughput at this build's optimization level. Pairs are generated
 * once with splitmix64 BEFORE timing starts, so the timed loop contains
 * only mulhi_u16 calls plus an xor into a volatile sink (the sink keeps
 * the compiler from folding the loop away). The PRNG step is therefore
 * NOT part of the measured time.
 */
static void test_throughput(void)
{
    for (uint32_t i = 0; i < 2 * NPAIRS; i++)
        tpairs[i] = (uint16_t)splitmix64();
    volatile uint64_t sink = 0;
    double best = 1e300;
    for (int pass = 0; pass < 5; pass++) {
        double t0 = now_ns();
        for (uint32_t i = 0; i < NPAIRS; i++)
            sink ^= (uint64_t)mulhi_u16(tpairs[2 * i], tpairs[2 * i + 1]);
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
    printf("mulhi_u16 differential test, build %s\n", BUILD_NAME);

    printf("[1/3] directed edge cases\n");
    fflush(stdout);
    test_directed();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64 "\n",
           cases, mismatches);

    printf("[2/3] exhaustive 16-bit x 16-bit pairs (4294967296 cases)\n");
    fflush(stdout);
    test_exhaustive();
    printf("  done: cases=%" PRIu64 " mismatches=%" PRIu64 "\n",
           cases, mismatches);

    printf("[3/3] throughput (PRNG pre-generated, excluded from timing)\n");
    test_throughput();

    printf("total verification cases: %" PRIu64 "\n", cases);
    printf("total mismatches: %" PRIu64 "\n", mismatches);
    printf("FNV-1a checksum of all result words: 0x%016" PRIx64 "\n", fnv);
    printf("RESULT: %s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
