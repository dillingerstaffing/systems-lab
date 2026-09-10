/*
 * Test for lab/81-wrapping-neg: wrapneg_u64 (~x + 1, no unary minus)
 * verified by differential testing against unary minus and against
 * two algebraic invariants, over exhaustive and random inputs.
 *
 *   [1] exhaustive: all 65,536 16-bit values, zero-extended to
 *       64 bits.
 *   [2] random: 1,000,000 splitmix64 64-bit words, fixed seed
 *       0x123456789ABCDEF0 (the proof-engine seed convention).
 *   [3] INT64_MIN edge: 0x8000000000000000 negates to itself;
 *       checked as a dedicated row because applying unary minus
 *       to INT64_MIN as a signed value is undefined, while the
 *       unsigned ~x+1 construction is fully defined and wraps.
 *       The contract is: wrapneg(INT64_MIN) == INT64_MIN, and the
 *       two invariants still hold (x + neg(x) == 0 mod 2^64 and
 *       neg(neg(x)) == x).
 *   [4] cross-build checksum: FNV-1a over every output must match
 *       across the -O0, -O2, ASan, and UBSan builds.
 *   [5] throughput of wrapneg_u64 at -O2 over ~100M timed values,
 *       PRNG pre-generated and excluded from the timed loop; one
 *       extra pass timed with the PRNG inside the loop for
 *       comparison (that number is a ceiling, it includes the
 *       generator cost).
 *
 * On every case the differential check compares wrapneg_u64(x)
 * against (uint64_t)(-(int64_t)x) (skipped for INT64_MIN, see [3]),
 * and the two invariants are verified: (x + wrapneg(x)) == 0 and
 * wrapneg(wrapneg(x)) == x, all in unsigned 64-bit arithmetic.
 *
 * Usage: ./test_wrapneg_o0 | ./test_wrapneg_o2 |
 *        ./test_wrapneg_asan | ./test_wrapneg_ubsan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "wrapneg.h"

#ifndef BUILD_NAME
#define BUILD_NAME "unknown"
#endif

/* splitmix64: the proof-engine fixed PRNG for test inputs. */
static uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* FNV-1a 64-bit over one 64-bit output word. */
static uint64_t fnv1a_step(uint64_t h, uint64_t v)
{
    h ^= v;
    h *= UINT64_C(1099511628211);
    return h;
}

static uint64_t total_cases;
static uint64_t total_mismatches;
static uint64_t invariant_violations;
static uint64_t fnv; /* FNV-1a checksum of every output, all cases. */

#define INT64_MIN_WORD UINT64_C(0x8000000000000000)

/* Check one input x against the ground truth. */
static void check(uint64_t x)
{
    uint64_t got = wrapneg_u64(x);
    total_cases++;

    /* Differential: unary minus is only defined for the signed
       interpretation when x is not INT64_MIN. */
    if (x != INT64_MIN_WORD) {
        int64_t sx = (int64_t)x;
        uint64_t expected = (uint64_t)(-sx);
        if (got != expected) {
            total_mismatches++;
            if (total_mismatches <= 8)
                printf("  MISMATCH x=%llx got=%llx unary-minus=%llx\n",
                       (unsigned long long)x,
                       (unsigned long long)got,
                       (unsigned long long)expected);
        }
    }

    /* Invariant 1: x + neg(x) == 0 (mod 2^64), unsigned arithmetic. */
    if (x + got != 0u) {
        invariant_violations++;
        if (invariant_violations <= 8)
            printf("  INVARIANT1 x=%llx got=%llx sum=%llx\n",
                   (unsigned long long)x,
                   (unsigned long long)got,
                   (unsigned long long)(x + got));
    }

    /* Invariant 2: neg(neg(x)) == x. */
    if (wrapneg_u64(got) != x) {
        invariant_violations++;
        if (invariant_violations <= 8)
            printf("  INVARIANT2 x=%llx got=%llx negneg=%llx\n",
                   (unsigned long long)x,
                   (unsigned long long)got,
                   (unsigned long long)wrapneg_u64(got));
    }

    fnv = fnv1a_step(fnv, got);
}

int main(void)
{
    printf("wrapneg differential test, build %s\n", BUILD_NAME);
    total_cases = 0;
    total_mismatches = 0;
    invariant_violations = 0;
    fnv = UINT64_C(14695981039346656037); /* FNV offset basis */

    /* [1] exhaustive 16-bit inputs, zero-extended to 64 bits. */
    printf("[1/5] exhaustive 16-bit inputs (zero-extended)\n");
    {
        uint64_t cases0 = total_cases;
        for (uint64_t i = 0; i < 65536u; i++)
            check(i);
        printf("  done: cases=%llu mismatches=%llu violations=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches,
               (unsigned long long)invariant_violations);
    }

    /* [2] 1,000,000 splitmix64 random 64-bit words. */
    printf("[2/5] random 64-bit words, splitmix64 seed 0x123456789ABCDEF0\n");
    {
        uint64_t cases0 = total_cases;
        uint64_t st = UINT64_C(0x123456789ABCDEF0);
        for (uint64_t i = 0; i < 1000000u; i++)
            check(splitmix64(&st));
        printf("  done: cases=%llu mismatches=%llu violations=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches,
               (unsigned long long)invariant_violations);
    }

    /* [3] INT64_MIN edge: negation wraps to itself.  Pin the
       contract explicitly. */
    printf("[3/5] INT64_MIN edge: wrapneg(0x8000000000000000)\n");
    {
        uint64_t got = wrapneg_u64(INT64_MIN_WORD);
        check(INT64_MIN_WORD); /* runs the two invariants too */
        printf("  wrapneg(INT64_MIN)=%llx (contract: equal to input)\n",
               (unsigned long long)got);
        if (got != INT64_MIN_WORD) {
            total_mismatches++;
            printf("  MISMATCH: edge contract broken\n");
        }
    }

    printf("[4/5] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan, UBSan\n");

    /* [5] throughput.  Inputs are pre-generated so the PRNG is
       excluded from the timed loop. */
    printf("[5/5] throughput (PRNG pre-generated, excluded from timing)\n");
    {
        const size_t NB = 1u << 20;
        const unsigned REPS = 100u;
        uint64_t *buf = malloc(NB * sizeof *buf);
        if (!buf) {
            printf("  malloc failed\n");
            return 1;
        }
        uint64_t st = UINT64_C(0x123456789ABCDEF0);
        for (size_t i = 0; i < NB; i++)
            buf[i] = splitmix64(&st);

        double best = 1e9;
        for (unsigned pass = 0; pass < 5; pass++) {
            uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i < NB; i++)
                    sink += wrapneg_u64(buf[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)NB * REPS);
            if (per < best)
                best = per;
            printf("  pass %u: %.3f ns/value (%.3f M values/s) sink=%llx\n",
                   pass, per * 1e9, 1.0 / (per * 1e6),
                   (unsigned long long)sink);
        }
        printf("  throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
               best * 1e9, 1.0 / (best * 1e6));

        /* One pass with the PRNG inside the timed loop.  This
           number includes the generator cost and is a ceiling. */
        {
            uint64_t st2 = UINT64_C(0x123456789ABCDEF0);
            uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i < NB; i++)
                    sink += wrapneg_u64(splitmix64(&st2));
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)NB * REPS);
            printf("  with PRNG in the timed loop (ceiling): %.3f ns/value (sink=%llx)\n",
                   per * 1e9, (unsigned long long)sink);
        }
        free(buf);
    }

    printf("total verification cases: %llu\n",
           (unsigned long long)total_cases);
    printf("total mismatches: %llu\n",
           (unsigned long long)total_mismatches);
    printf("invariant violations: %llu\n",
           (unsigned long long)invariant_violations);
    printf("FNV-1a checksum of all outputs: 0x%llx\n",
           (unsigned long long)fnv);
    if (total_mismatches == 0 && invariant_violations == 0) {
        printf("RESULT: PASS\n");
        return 0;
    }
    printf("RESULT: FAIL\n");
    return 1;
}
