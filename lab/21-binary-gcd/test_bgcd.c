/*
 * Differential test for bgcd32.
 *
 * Oracle: the naive Euclid's algorithm, written out by hand (no library
 * gcd), with gcd(0, 0) = 0 so oracle and implementation agree on every
 * input. Verification plan:
 *
 *   1. Exhaustive all-pairs over 0..1023 (1,048,576 pairs), which covers
 *      0, 1, powers of 2, and all small edge cases, including (0, 0),
 *      asserted explicitly.
 *   2. 1,000,000 pairs of full 32-bit values from a fixed-seed
 *      xorshift64* PRNG, fully reproducible.
 *
 * Every case compares bgcd32 against the oracle; a 64-bit FNV-1a
 * checksum accumulates all outputs and must be identical across the
 * -O0, -O2, and ASan+UBSan builds, proving the tested behavior does
 * not depend on optimization level or instrumentation.
 */
#include "bgcd.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RAND_CASES 1000000u

/* Fixed seed, stated here so the random stream is reproducible. */
#define SEED 0x2545F4914F6CDD1DULL

static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

/* Naive Euclid reference; gcd(0, 0) = 0 by definition of the loop. */
static uint32_t ref_gcd(uint32_t a, uint32_t b)
{
    while (b != 0u) {
        uint32_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static uint64_t checksum;

static void check(uint32_t a, uint32_t b, unsigned long *mismatches)
{
    uint32_t got = bgcd32(a, b);
    uint32_t want = ref_gcd(a, b);

    if (got != want) {
        printf("MISMATCH: gcd(%u, %u): bgcd=%u ref=%u\n",
               a, b, got, want);
        ++*mismatches;
    }
    checksum ^= got;
    checksum *= 1099511628211ULL;
}

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

int main(void)
{
    unsigned long mismatches = 0;
    uint32_t a, b, i;
    uint64_t t0, t1;

    /* Explicit contract check: gcd(0, 0) = 0. */
    if (bgcd32(0u, 0u) != 0u || ref_gcd(0u, 0u) != 0u) {
        printf("FAIL: gcd(0, 0) is not 0\n");
        return 1;
    }

    /* Sweep 1: exhaustive all-pairs over 0..1023. */
    for (a = 0; a < 1024; ++a) {
        for (b = 0; b < 1024; ++b)
            check(a, b, &mismatches);
    }

    /* Sweep 2: 1,000,000 full-range 32-bit pairs, timed. */
    rng_state = SEED;
    t0 = now_ns();
    for (i = 0; i < RAND_CASES; ++i) {
        a = (uint32_t)rng_next();
        b = (uint32_t)(rng_next() >> 32);
        check(a, b, &mismatches);
    }
    t1 = now_ns();

    printf("exhaustive_pairs=1048576 random_pairs=%u total=%u mismatches=%lu\n",
           RAND_CASES, 1048576u + RAND_CASES, mismatches);
    printf("checksum=%" PRIu64 "\n", checksum);
    printf("random_sweep: pairs=%u ns_total=%" PRIu64 " ns_per_pair=%.2f\n",
           RAND_CASES, t1 - t0,
           (double)(t1 - t0) / (double)RAND_CASES);

    if (mismatches != 0) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
