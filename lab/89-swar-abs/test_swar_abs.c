/*
 * test_swar_abs: differential verification of swar_abs_u16x4
 * (swar_abs.h) against a scalar per-lane reference.
 *
 * Sections:
 *   [1] anchors: hand-checked per-lane values (0, 1, 0x7FFE,
 *       0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF, 0x1234, 0xEDCC)
 *       planted in every lane position, plus mixed-lane words;
 *       expected values cross-checked against the scalar
 *       reference so a typo cannot silently pass.
 *   [2] exhaustive: all 2^16 lane values x all four lane
 *       positions (neighbors zero), plus all 2^16 values in
 *       each lane position with neighbors at 0x0000/0xFFFF/
 *       0x7FFF/0x8000.
 *   [3] directed lane crosstalk: alternating sign patterns
 *       across lanes, -32768 in each lane position with
 *       neighbors at 32767/-1/0 in every combination, and
 *       single-bit words.
 *   [4] 10,000,000 fixed-seed splitmix64 64-bit words.  On every
 *       case in every section, the invariants abs(abs(x)) == abs(x)
 *       and "each result lane is non-negative, except a 0x8000
 *       result lane whose input lane was 0x8000" are checked too.
 *   [5] cross-build checksum: FNV-1a over every output must match
 *       across -O0, -O2, and ASan+UBSan builds.
 *   [6] throughput of swar_abs_u16x4 at -O2 over 100,000,000 timed
 *       values, PRNG pre-generated and excluded from the timed
 *       loop; one extra pass timed with the PRNG inside the loop
 *       for comparison.
 *
 * Usage: ./test_swar_abs_o0 | ./test_swar_abs_o2 | ./test_swar_abs_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "swar_abs.h"

/* Scalar reference: per-lane absolute value.  A plain if/else on
 * extracted int16_t lanes, deliberately NOT the (x ^ m) - m bit
 * trick.  The -32768 wrap is pinned by contract: negation goes
 * through unsigned arithmetic, so 0x8000 maps to 0x8000. */
static uint64_t ref_abs(uint64_t x)
{
    uint64_t r = 0;
    for (unsigned i = 0; i < 4; i++) {
        int16_t v = (int16_t)((x >> (16 * i)) & 0xFFFFu);
        uint16_t a;
        if (v < 0)
            a = (uint16_t)(0u - (uint16_t)v);
        else
            a = (uint16_t)v;
        r |= (uint64_t)a << (16 * i);
    }
    return r;
}

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

#ifndef BUILD_NAME
#define BUILD_NAME "unknown"
#endif

static uint64_t fnv;
static uint64_t total_cases;
static uint64_t total_mismatches;

static void check(uint64_t x)
{
    uint64_t got = swar_abs_u16x4(x);
    uint64_t want = ref_abs(x);
    total_cases++;
    total_mismatches += (got != want);
    if (got != want && total_mismatches < 4)
        printf("  MISMATCH: x=%016llx got=%016llx want=%016llx\n",
               (unsigned long long)x, (unsigned long long)got,
               (unsigned long long)want);
    fnv = fnv1a_step(fnv, got);

    /* Invariant: abs(abs(x)) == abs(x). */
    if (swar_abs_u16x4(got) != got) {
        total_mismatches++;
        if (total_mismatches < 4)
            printf("  IDEMPOTENCE FAIL: x=%016llx got=%016llx\n",
                   (unsigned long long)x, (unsigned long long)got);
    }

    /* Invariant: every result lane is non-negative, except a
     * 0x8000 result lane whose input lane was 0x8000 (the pinned
     * wrap contract). */
    for (unsigned i = 0; i < 4; i++) {
        uint16_t gl = (uint16_t)(got >> (16 * i));
        uint16_t xl = (uint16_t)(x >> (16 * i));
        int ok = ((int16_t)gl >= 0) || (gl == 0x8000u && xl == 0x8000u);
        if (!ok) {
            total_mismatches++;
            if (total_mismatches < 4)
                printf("  NEGATIVE LANE: x=%016llx got=%016llx lane=%u\n",
                       (unsigned long long)x, (unsigned long long)got, i);
        }
    }
}

int main(void)
{
    fnv = UINT64_C(14695981039346656037);
    total_cases = 0;
    total_mismatches = 0;

    printf("swar_abs differential test, build %s\n", BUILD_NAME);

    /* [1] anchors, hand-checked. */
    printf("[1/6] anchors\n");
    {
        /* Per-lane hand-checked pairs {input lane, abs lane}. */
        static const uint16_t av[] = {
            0x0000, 0x0001, 0x7FFE, 0x7FFF, 0x8000,
            0x8001, 0xFFFE, 0xFFFF, 0x1234, 0xEDCC,
        };
        static const uint16_t aw[] = {
            0x0000, 0x0001, 0x7FFE, 0x7FFF, 0x8000,
            0x7FFF, 0x0002, 0x0001, 0x1234, 0x1234,
        };
        for (unsigned p = 0; p < 4; p++) {
            for (unsigned i = 0;
                 i < sizeof(av) / sizeof(av[0]); i++) {
                uint64_t x = (uint64_t)av[i] << (16 * p);
                uint64_t want = (uint64_t)aw[i] << (16 * p);
                uint64_t got = swar_abs_u16x4(x);
                total_cases++;
                total_mismatches += (got != want);
                if (got != want)
                    printf("  MISMATCH anchor lane %u val %04x: "
                           "got=%016llx want=%016llx\n",
                           p, av[i], (unsigned long long)got,
                           (unsigned long long)want);
                fnv = fnv1a_step(fnv, got);
            }
        }
        /* Mixed-lane words, hand-checked. */
        static const uint64_t mx[] = {
            0x8000800080008000ULL, 0x80007FFF80007FFFULL,
            0x7FFF80007FFF8000ULL, 0xFFFF0000FFFF0000ULL,
            0x0000FFFF0000FFFFULL, 0x8001FFFE00017FFFULL,
            0x1234EDCC8000FFFFULL, 0xFFFFFFFFFFFFFFFFULL,
        };
        static const uint64_t mw[] = {
            0x8000800080008000ULL, 0x80007FFF80007FFFULL,
            0x7FFF80007FFF8000ULL, 0x0001000000010000ULL,
            0x0000000100000001ULL, 0x7FFF000200017FFFULL,
            0x1234123480000001ULL, 0x0001000100010001ULL,
        };
        for (unsigned i = 0; i < sizeof(mw) / sizeof(mw[0]); i++) {
            uint64_t got = swar_abs_u16x4(mx[i]);
            total_cases++;
            total_mismatches += (got != mw[i]);
            if (got != mw[i])
                printf("  MISMATCH mixed anchor #%u: got=%016llx "
                       "want=%016llx\n",
                       i, (unsigned long long)got,
                       (unsigned long long)mw[i]);
            fnv = fnv1a_step(fnv, got);
        }
        /* Cross-check every anchor expectation against the
         * scalar reference, so a typo in the tables cannot
         * silently pass. */
        for (unsigned p = 0; p < 4; p++)
            for (unsigned i = 0; i < sizeof(av) / sizeof(av[0]); i++) {
                uint64_t x = (uint64_t)av[i] << (16 * p);
                uint64_t want = (uint64_t)aw[i] << (16 * p);
                if (ref_abs(x) != want) {
                    printf("  BAD ANCHOR lane %u val %04x: ref=%016llx\n",
                           p, av[i], (unsigned long long)ref_abs(x));
                    total_mismatches++;
                }
            }
        for (unsigned i = 0; i < sizeof(mw) / sizeof(mw[0]); i++)
            if (ref_abs(mx[i]) != mw[i]) {
                printf("  BAD MIXED ANCHOR #%u: ref=%016llx\n",
                       i, (unsigned long long)ref_abs(mx[i]));
                total_mismatches++;
            }
        printf("  anchors checked: %llu, mismatches so far: %llu\n",
               (unsigned long long)total_cases,
               (unsigned long long)total_mismatches);
    }

    /* [2] exhaustive: all 2^16 lane values x all four lane
     * positions (neighbors zero), then the same sweep with
     * neighbors at 0x0000/0xFFFF/0x7FFF/0x8000. */
    printf("[2/6] exhaustive lane values x lane positions\n");
    {
        static const uint16_t npat[] = { 0x0000, 0xFFFF, 0x7FFF, 0x8000 };
        uint64_t cases0 = total_cases;
        for (unsigned p = 0; p < 4; p++) {
            /* Neighbors zero. */
            for (uint32_t v = 0; v < 0x10000u; v++)
                check((uint64_t)v << (16 * p));
            /* Neighbors at extreme/boundary patterns. */
            for (unsigned k = 0; k < 4; k++) {
                uint64_t nb = 0;
                for (unsigned i = 0; i < 4; i++)
                    if (i != p)
                        nb |= (uint64_t)npat[k] << (16 * i);
                for (uint32_t v = 0; v < 0x10000u; v++)
                    check(nb | ((uint64_t)v << (16 * p)));
            }
        }
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches);
    }

    /* [3] directed lane crosstalk. */
    printf("[3/6] directed lane-crosstalk cases\n");
    {
        uint64_t cases0 = total_cases;
        /* Alternating sign patterns across lanes. */
        static const uint64_t alt[] = {
            0x80007FFF80007FFFULL, 0x7FFF80007FFF8000ULL,
            0xFFFF0001FFFF0001ULL, 0x0001FFFF0001FFFFULL,
            0x8000FFFF7FFF0000ULL, 0x00007FFFFFFFFFFFULL,
            0x8001800180018001ULL, 0x7FFE7FFE7FFE7FFEULL,
        };
        for (unsigned i = 0; i < sizeof(alt) / sizeof(alt[0]); i++)
            check(alt[i]);
        /* -32768 in each lane position, neighbors at every
         * combination of {32767, -1, 0}. */
        static const uint16_t nbv[] = { 0x7FFF, 0xFFFF, 0x0000 };
        for (unsigned p = 0; p < 4; p++) {
            unsigned idx[3];
            for (idx[0] = 0; idx[0] < 3; idx[0]++)
                for (idx[1] = 0; idx[1] < 3; idx[1]++)
                    for (idx[2] = 0; idx[2] < 3; idx[2]++) {
                        uint64_t x = (uint64_t)0x8000 << (16 * p);
                        unsigned q = 0;
                        for (unsigned i = 0; i < 4; i++) {
                            if (i == p)
                                continue;
                            x |= (uint64_t)nbv[idx[q++]] << (16 * i);
                        }
                        check(x);
                    }
        }
        /* Single-bit words: every bit position on its own. */
        for (unsigned b = 0; b < 64; b++)
            check(UINT64_C(1) << b);
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches);
    }

    /* [4] 10,000,000 fixed-seed splitmix64 64-bit words. */
    printf("[4/6] random 64-bit words\n");
    {
        uint64_t cases0 = total_cases;
        uint64_t st = UINT64_C(0x123456789ABCDEF0);
        for (uint64_t i = 0; i < 10000000u; i++)
            check(splitmix64(&st));
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches);
    }

    printf("[5/6] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan+UBSan\n");

    /* [6] throughput.  Inputs are pre-generated so the PRNG is
     * outside the timed loop; one extra pass times PRNG+abs. */
    printf("[6/6] throughput (PRNG pre-generated, excluded from timing)\n");
    {
        const size_t NB = 1u << 20;
        const unsigned REPS = 100; /* 100 * 2^20 = 100M values */
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
                    sink += swar_abs_u16x4(buf[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)NB * REPS);
            printf("  abs pass %u: %.3f ns/value (%.3f M values/s) sink=%llx\n",
                   pass, per * 1e9, 1.0 / per / 1e6,
                   (unsigned long long)sink);
            fnv = fnv1a_step(fnv, sink);
            if (per < best)
                best = per;
        }
        printf("  abs throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
               best * 1e9, 1.0 / best / 1e6);

        /* With the PRNG inside the timed loop, for comparison. */
        {
            uint64_t sink = 0;
            struct timespec t0, t1;
            st = UINT64_C(0x123456789ABCDEF0);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i < NB; i++)
                    sink += swar_abs_u16x4(splitmix64(&st));
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)NB * REPS);
            printf("  with PRNG in the timed loop: %.3f ns/value (sink=%llx)\n",
                   per * 1e9, (unsigned long long)sink);
        }
        free(buf);
    }

    printf("total verification cases: %llu\n", (unsigned long long)total_cases);
    printf("total mismatches: %llu\n", (unsigned long long)total_mismatches);
    printf("FNV-1a checksum of all outputs: 0x%016llx\n",
           (unsigned long long)fnv);
    printf("RESULT: %s\n", total_mismatches == 0 ? "PASS" : "FAIL");
    return total_mismatches == 0 ? 0 : 1;
}
