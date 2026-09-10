/*
 * lab/83: count trailing zeros via the ctzless identity.
 *
 * Unit under test: ctz64_identity() from ctz.c, built only from
 * ctz(x) = popcount((x ^ (x-1)) >> 1) with a hand-rolled SWAR popcount.
 * The differential oracle is an independent shift loop below (not a
 * compiler intrinsic): the loop peels off zero bits one at a time, so
 * its count equals the number of trailing zeros by direct inspection.
 *
 * Phases:
 *  1. Single-bit words: x = 1<<k for k = 0..63, ctz must be k. These
 *     are the extreme boundary cases (all zeros below bit k).
 *  2. Exhaustive 16-bit: every nonzero 16-bit value (1..65535) as a
 *     64-bit word, identity vs shift loop, zero mismatches allowed.
 *  3. 1,000,000 fixed-seed splitmix64 64-bit values (zeros redrawn so
 *     the domain stays nonzero), identity vs shift loop, zero
 *     mismatches allowed.
 *  4. Contract pin for x = 0: out of contract by definition; the row
 *     records the observed return value and asserts nothing about it
 *     beyond being a uint32.
 *  5. Timing: -O2 build, best-of-5 ns/value over the 1,000,000 timed
 *     values. The splitmix64 PRNG step is NOT inside the timed region
 *     (values are pre-generated); stated explicitly.
 *
 * An FNV-1a checksum folded over every case input and result must be
 * identical across the -O0, -O2, and ASan+UBSan builds.
 */
#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ctz.h"

#define NCASES 1000000UL
#define SEED 0x123456789ABCDEF0ULL   /* fixed splitmix64 seed (proof-engine convention) */
#define NRUNS 5

/* splitmix64, fixed seed. */
static uint64_t sm_state;

static void sm_seed(uint64_t seed)
{
    sm_state = seed;
}

static uint64_t sm_next(void)
{
    uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Independent reference: peel trailing zero bits one at a time.
 * For nonzero x the loop stops exactly when the lowest set bit is
 * reached, so the count is the trailing-zero count by construction. */
static uint32_t ctz64_ref(uint64_t x)
{
    uint32_t n = 0;
    while ((x & 1ULL) == 0) {
        x >>= 1;
        n++;
    }
    return n;
}

/* FNV-1a 64, folded over every case: the 8 input bytes and 1 result byte. */
static uint64_t fnv;

static void fnv_fold_u64(uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        fnv ^= (uint8_t)(v >> (8 * i));
        fnv *= 0x100000001B3ULL;
    }
}

static void fnv_fold_u8(uint8_t v)
{
    fnv ^= v;
    fnv *= 0x100000001B3ULL;
}

static int failures = 0;

/* Compare identity vs reference on one value; fold into fnv; count mismatches. */
static uint64_t mismatches = 0;

static void check(uint64_t x)
{
    uint32_t got = ctz64_identity(x);
    uint32_t want = ctz64_ref(x);
    if (got != want)
        mismatches++;
    fnv_fold_u64(x);
    fnv_fold_u8((uint8_t)got);
}

static uint64_t timed_cases(void);

int main(void)
{
    /* ---- phase 1: single-bit boundary words ---- */
    for (int k = 0; k < 64; k++) {
        uint64_t x = 1ULL << k;
        check(x);
    }
    printf("ok   single-bit words k=0..63: mismatches %" PRIu64 "\n", mismatches);

    /* ---- phase 2: exhaustive nonzero 16-bit inputs ---- */
    for (uint32_t v = 1; v < 65536; v++)
        check(v);
    printf("ok   exhaustive 16-bit (65535 values): mismatches %" PRIu64 "\n",
           mismatches);

    /* ---- phase 3: 1,000,000 fixed-seed random 64-bit values ---- */
    sm_seed(SEED);
    uint64_t zeros_skipped = 0;
    for (uint64_t i = 0; i < NCASES; i++) {
        uint64_t x = sm_next();
        while (x == 0) {          /* keep the domain nonzero per contract */
            x = sm_next();
            zeros_skipped++;
        }
        check(x);
    }
    printf("random 64-bit cases: %" PRIu64 " (zeros redrawn: %" PRIu64 ")\n",
           NCASES, zeros_skipped);
    printf("identity vs shift-loop mismatches (all phases): %" PRIu64 "\n",
           mismatches);
    printf("fnv64 over per-case (input, result): 0x%016" PRIx64 "\n", fnv);
    if (mismatches)
        failures++;

    /* ---- phase 4: out-of-contract x = 0 pin ---- */
    {
        uint32_t r = ctz64_identity(0);
        printf("contract pin: ctz64_identity(0) = %u (x=0 is out of contract;"
               " observed only, no guarantee)\n", r);
    }

    /* ---- phase 5: throughput, best of 5, -O2 ---- */
    double best = timed_cases();
    printf("throughput: %.2f ns/value (best of %d; PRNG not in timed region)\n",
           best, NRUNS);

    if (failures) {
        printf("RESULT: FAILURES PRESENT\n");
        return 1;
    }
    printf("RESULT: ALL TESTS PASSED\n");
    return 0;
}

/* Pre-generate the timed corpus with the same fixed seed (fresh
 * generator, so the timed values match phase 3 exactly), then time
 * only the identity loop. Accumulate into a volatile sink so the loop
 * cannot be dead-coded away. */
static uint64_t timed_cases(void)
{
    uint64_t *vals = malloc(NCASES * sizeof *vals);
    if (!vals) {
        printf("FAIL malloc\n");
        failures++;
        return 0.0;
    }
    sm_seed(SEED);
    for (uint64_t i = 0; i < NCASES; i++) {
        uint64_t x = sm_next();
        while (x == 0)
            x = sm_next();
        vals[i] = x;
    }

    volatile uint32_t sink = 0;
    double best_ns = 1e18;
    for (int r = 0; r < NRUNS; r++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (uint64_t i = 0; i < NCASES; i++)
            sink += ctz64_identity(vals[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9
                  + (double)(t1.tv_nsec - t0.tv_nsec);
        if (ns / (double)NCASES < best_ns)
            best_ns = ns / (double)NCASES;
    }
    printf("timed sink (prevents dead-code elimination): %u\n",
           (uint32_t)sink);
    free(vals);
    return best_ns;
}
