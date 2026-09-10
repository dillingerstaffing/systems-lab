/*
 * test_saturate_u16: differential verification of saturate_u16
 * (saturate_u16.h) against an independent if/else reference.
 *
 * Sections:
 *   [1] anchors: hand-checked values (INT32_MIN, -65537, -65536,
 *       -2, -1, 0, 1, 65534, 65535, 65536, 65537, INT32_MAX);
 *       every expected value cross-checked against the
 *       independent reference so a typo cannot silently pass.
 *   [2] exhaustive: saturate_u16((int32_t)u) for every u in
 *       [0, 2^32), i.e. all 4,294,967,296 int32 inputs,
 *       differential against the reference.  On every case the
 *       invariants "saturate(saturate(x)) == saturate(x)" and
 *       "the output is monotone non-decreasing in x on each of
 *       the two monotone runs u in [0, 2^31) and u in
 *       [2^31, 2^32)" are checked too.
 *   [3] cross-build checksum: FNV-1a over every output must
 *       match across -O0, -O2, and ASan+UBSan builds.
 *   [4] throughput of saturate_u16 at -O2 over 100,000,000 timed
 *       values, PRNG pre-generated and excluded from the timed
 *       loop; one extra pass timed with the PRNG inside the loop
 *       for comparison.
 *
 * Usage: ./test_saturate_u16_o0 | ./test_saturate_u16_o2 |
 *        ./test_saturate_u16_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "saturate_u16.h"

/* Independent reference: plain if/else clamping, deliberately NOT
 * the sign-mask select. */
static uint16_t ref_saturate(int32_t x)
{
    if (x < 0)
        return 0;
    if (x > 65535)
        return 65535;
    return (uint16_t)x;
}

/* splitmix64: the proof-engine fixed PRNG for test inputs. */
static uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* FNV-1a 64-bit over one 16-bit output value. */
static uint64_t fnv1a_step(uint64_t h, uint16_t v)
{
    h ^= v;
    h *= UINT64_C(1099511628211);
    return h;
}

#ifndef BUILD_NAME
#define BUILD_NAME "unknown"
#endif

static uint64_t fnv;
static uint64_t total_cases;
static uint64_t total_mismatches;

int main(void)
{
    fnv = UINT64_C(14695981039346656037);
    total_cases = 0;
    total_mismatches = 0;

    printf("saturate_u16 differential test, build %s\n", BUILD_NAME);

    /* [1] anchors, hand-checked. */
    printf("[1/4] anchors\n");
    {
        static const int32_t ax[] = {
            INT32_MIN, -65537, -65536, -2, -1,
            0, 1, 65534, 65535, 65536, 65537, INT32_MAX,
        };
        static const uint16_t aw[] = {
            0, 0, 0, 0, 0,
            0, 1, 65534, 65535, 65535, 65535, 65535,
        };
        for (unsigned i = 0;
             i < sizeof(ax) / sizeof(ax[0]); i++) {
            uint16_t got = saturate_u16(ax[i]);
            total_cases++;
            total_mismatches += (got != aw[i]);
            if (got != aw[i])
                printf("  MISMATCH anchor x=%d: got=%u want=%u\n",
                       ax[i], got, aw[i]);
            /* Cross-check every anchor expectation against the
             * independent reference, so a typo in the table
             * cannot silently pass. */
            if (ref_saturate(ax[i]) != aw[i]) {
                printf("  BAD ANCHOR x=%d: ref=%u\n",
                       ax[i], ref_saturate(ax[i]));
                total_mismatches++;
            }
            fnv = fnv1a_step(fnv, got);
        }
        printf("  anchors checked: %llu, mismatches so far: %llu\n",
               (unsigned long long)total_cases,
               (unsigned long long)total_mismatches);
    }

    /* [2] exhaustive: all 2^32 uint32 inputs, i.e. every
     * int32 value, differential against the reference. */
    printf("[2/4] exhaustive all 2^32 inputs\n");
    {
        uint64_t cases0 = total_cases;
        uint16_t prev_out = 0;
        for (uint64_t u = 0; u < UINT64_C(0x100000000); u++) {
            int32_t x = (int32_t)(uint32_t)u;
            uint16_t got = saturate_u16(x);
            uint16_t want = ref_saturate(x);
            total_cases++;
            uint64_t bad = (got != want);
            /* Invariant: idempotence, the output is already in
             * range so clamping it again is a fixed point. */
            bad += (saturate_u16((int32_t)got) != got);
            /* Invariant: monotone non-decreasing in x on each
             * of the two runs u in [0, 2^31) and
             * u in [2^31, 2^32); reset the running previous
             * value at the wrap point u = 2^31. */
            if (u == UINT64_C(0x80000000))
                prev_out = got;
            else if (u != 0)
                bad += (got < prev_out);
            prev_out = got;
            total_mismatches += (bad != 0);
            if (bad && total_mismatches < 4)
                printf("  MISMATCH/INVARIANT FAIL: x=%d "
                       "(u=%llu) got=%u want=%u\n",
                       x, (unsigned long long)u, got, want);
            fnv = fnv1a_step(fnv, got);
        }
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches);
    }

    printf("[3/4] cross-build checksum: compare the FNV-1a line "
           "across -O0, -O2, ASan+UBSan\n");

    /* [4] throughput.  Inputs are pre-generated so the PRNG is
     * outside the timed loop; one extra pass times PRNG+sat. */
    printf("[4/4] throughput (PRNG pre-generated, excluded from "
           "timing)\n");
    {
        const size_t NB = 1u << 20;
        const unsigned REPS = 100; /* 100 * 2^20 = 100M values */
        uint32_t *buf = malloc(NB * sizeof *buf);
        if (!buf) {
            printf("  malloc failed\n");
            return 1;
        }
        uint64_t st = UINT64_C(0x123456789ABCDEF0);
        for (size_t i = 0; i < NB; i++)
            buf[i] = (uint32_t)splitmix64(&st);

        double best = 1e9;
        for (unsigned pass = 0; pass < 5; pass++) {
            uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i < NB; i++)
                    sink += saturate_u16((int32_t)buf[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)NB * REPS);
            printf("  sat pass %u: %.3f ns/value (%.3f M values/s) "
                   "sink=%llx\n",
                   pass, per * 1e9, 1.0 / per / 1e6,
                   (unsigned long long)sink);
            fnv = fnv1a_step(fnv, (uint16_t)sink);
            if (per < best)
                best = per;
        }
        printf("  sat throughput best of 5: %.3f ns/value "
               "(%.3f M values/s)\n",
               best * 1e9, 1.0 / best / 1e6);

        /* With the PRNG inside the timed loop, for comparison. */
        {
            uint64_t sink = 0;
            struct timespec t0, t1;
            st = UINT64_C(0x123456789ABCDEF0);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i < NB; i++)
                    sink += saturate_u16(
                        (int32_t)(uint32_t)splitmix64(&st));
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)NB * REPS);
            printf("  with PRNG in the timed loop: %.3f ns/value "
                   "(sink=%llx)\n",
                   per * 1e9, (unsigned long long)sink);
        }
        free(buf);
    }

    printf("total verification cases: %llu\n",
           (unsigned long long)total_cases);
    printf("total mismatches: %llu\n",
           (unsigned long long)total_mismatches);
    printf("FNV-1a checksum of all outputs: 0x%016llx\n",
           (unsigned long long)fnv);
    printf("RESULT: %s\n", total_mismatches == 0 ? "PASS" : "FAIL");
    return total_mismatches == 0 ? 0 : 1;
}
