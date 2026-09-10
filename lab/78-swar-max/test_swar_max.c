/*
 * test_swar_max: differential verification of swar_max2_u16x4 and
 * swar_max_u16x4 (swar_max.h) against scalar references.
 *
 * Sections:
 *   [1] anchors: hand-checked pairwise and horizontal cases,
 *       including the 0x7FFF/0x8000 unsigned boundary in every
 *       lane position and mixed-lane words.
 *   [2] exhaustive: all 2^32 (a, b) 16-bit pairs, replicated to
 *       all four lanes each iteration, so every lane position
 *       sees the full pair space; each lane checked against the
 *       scalar max(a, b).
 *   [3] directed lane crosstalk: for each lane position, the
 *       test lane sweeps all 2^16 values while neighboring lanes
 *       sit at 0x0000/0xFFFF/0x7FFF/0x8000 (and the other word's
 *       neighbors at the complementary pattern), proving the
 *       guard bits do not leak between lanes.
 *   [4] horizontal: directed words (max planted in each lane
 *       position, ties, extremes) plus 10,000,000 fixed-seed
 *       splitmix64 64-bit words, checked against the scalar max
 *       of the four lanes.
 *   [5] cross-build checksum: FNV-1a over every output must match
 *       across -O0, -O2, and ASan+UBSan builds.
 *   [6] throughput of swar_max_u16x4 (and swar_max2_u16x4) at
 *       -O2 over 100,000,000 timed values, PRNG pre-generated and
 *       excluded from the timed loop; one extra pass timed with
 *       the PRNG inside the loop for comparison.
 *
 * Usage: ./test_swar_max_o0 | ./test_swar_max_o2 | ./test_swar_max_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "swar_max.h"

/* Replicate a 16-bit value to all four lanes.  a < 2^16, so the
 * multiply cannot carry between the 16-bit fields.  A macro so it
 * can also appear in static initializers. */
#define REP16V(a) ((uint64_t)(a) * UINT64_C(0x0001000100010001))

/* Scalar reference: per-lane unsigned max. */
static uint64_t ref_max2(uint64_t x, uint64_t y)
{
    uint64_t r = 0;
    for (unsigned i = 0; i < 4; i++) {
        uint32_t a = (uint32_t)((x >> (16 * i)) & 0xFFFFu);
        uint32_t b = (uint32_t)((y >> (16 * i)) & 0xFFFFu);
        uint32_t m = a >= b ? a : b;
        r |= (uint64_t)m << (16 * i);
    }
    return r;
}

/* Scalar reference: horizontal max of the four lanes. */
static uint16_t ref_hmax(uint64_t x)
{
    uint32_t best = 0;
    for (unsigned i = 0; i < 4; i++) {
        uint32_t v = (uint32_t)((x >> (16 * i)) & 0xFFFFu);
        if (v > best)
            best = v;
    }
    return (uint16_t)best;
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

static void check2(uint64_t x, uint64_t y)
{
    uint64_t got = swar_max2_u16x4(x, y);
    uint64_t want = ref_max2(x, y);
    total_cases++;
    total_mismatches += (got != want);
    if (got != want && total_mismatches < 4)
        printf("  MISMATCH max2: x=%016llx y=%016llx got=%016llx want=%016llx\n",
               (unsigned long long)x, (unsigned long long)y,
               (unsigned long long)got, (unsigned long long)want);
    fnv = fnv1a_step(fnv, got);
}

static void checkh(uint64_t x)
{
    uint16_t got = swar_max_u16x4(x);
    uint16_t want = ref_hmax(x);
    total_cases++;
    total_mismatches += (got != want);
    if (got != want && total_mismatches < 4)
        printf("  MISMATCH hmax: x=%016llx got=%04x want=%04x\n",
               (unsigned long long)x, got, want);
    fnv = fnv1a_step(fnv, got);
}

int main(void)
{
    fnv = UINT64_C(14695981039346656037);
    total_cases = 0;
    total_mismatches = 0;

    printf("swar_max differential test, build %s\n", BUILD_NAME);

    /* [1] anchors, hand-checked. */
    printf("[1/6] anchors\n");
    {
        /* Pairwise anchors: {x, y, expected per-lane max}. */
        static const uint64_t ax[] = {
            0x0000000000000000ULL, 0x0000000000000000ULL,
            REP16V(0xFFFF), REP16V(0x0000),
            REP16V(0x0000), REP16V(0xFFFF),
            REP16V(0x7FFF), REP16V(0x8000),
            REP16V(0x8000), REP16V(0x7FFF),
            REP16V(0x8001), REP16V(0x8000),
            REP16V(0x8000), REP16V(0x8001),
            0x0001000200030004ULL, 0x0004000300020001ULL,
            0xFFFF0000FFFF0000ULL, 0x0000FFFF0000FFFFULL,
            0x80007FFF80007FFFULL, 0x7FFF80007FFF8000ULL,
            0x00000000FFFFFFFFULL, 0xFFFFFFFF00000000ULL,
        };
        static const uint64_t awant[] = {
            0x0000000000000000ULL,
            REP16V(0xFFFF),
            REP16V(0xFFFF),
            REP16V(0x8000),
            REP16V(0x8000),
            REP16V(0x8001),
            REP16V(0x8001),
            0x0004000300030004ULL,
            0xFFFFFFFFFFFFFFFFULL,
            0x8000800080008000ULL,
            0xFFFFFFFFFFFFFFFFULL,
        };
        for (unsigned i = 0; i < sizeof(awant) / sizeof(awant[0]); i++) {
            uint64_t got = swar_max2_u16x4(ax[2 * i], ax[2 * i + 1]);
            total_cases++;
            total_mismatches += (got != awant[i]);
            if (got != awant[i])
                printf("  MISMATCH anchor2 #%u: got=%016llx want=%016llx\n",
                       i, (unsigned long long)got,
                       (unsigned long long)awant[i]);
            fnv = fnv1a_step(fnv, got);
        }
        /* Horizontal anchors: {x, expected max}. */
        static const uint64_t hx[] = {
            0x0004000300020001ULL, 0x0001000200030004ULL,
            0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
            0x8000000000000000ULL, 0x0000000000008000ULL,
            0x7FFF7FFF7FFF7FFFULL, 0x00017FFF8000FFFFULL,
            0x800080007FFF7FFFULL, 0x0000000080000000ULL,
        };
        static const uint16_t hwant[] = {
            4, 4, 0, 0xFFFF, 0x8000, 0x8000, 0x7FFF, 0xFFFF,
            0x8000, 0x8000,
        };
        for (unsigned i = 0; i < sizeof(hwant) / sizeof(hwant[0]); i++)
            checkh(hx[i]);
        /* Cross-check the horizontal anchors' expected values
         * against the scalar reference, so a typo in hwant
         * cannot silently pass. */
        for (unsigned i = 0; i < sizeof(hwant) / sizeof(hwant[0]); i++) {
            if (ref_hmax(hx[i]) != hwant[i]) {
                printf("  BAD ANCHOR #%u: ref=%04x hwant=%04x\n",
                       i, ref_hmax(hx[i]), hwant[i]);
                total_mismatches++;
            }
        }
        printf("  anchors checked: %llu, mismatches so far: %llu\n",
               (unsigned long long)total_cases,
               (unsigned long long)total_mismatches);
    }

    /* [2] exhaustive 2^32 (a, b) pairs, replicated to all lanes. */
    printf("[2/6] exhaustive 16-bit pairs, replicated to all four lanes\n");
    {
        uint64_t cases0 = total_cases;
        for (uint64_t n = 0; n < UINT64_C(0x100000000); n++) {
            uint32_t a = (uint32_t)(n & 0xFFFFu);
            uint32_t b = (uint32_t)((n >> 16) & 0xFFFFu);
            uint64_t x = REP16V(a);
            uint64_t y = REP16V(b);
            uint64_t got = swar_max2_u16x4(x, y);
            uint32_t r = a >= b ? a : b;
            uint64_t want = REP16V(r);
            total_cases++;
            total_mismatches += (got != want);
            fnv = fnv1a_step(fnv, got);
        }
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches);
    }

    /* [3] directed lane crosstalk: sweep one lane through all
     * 2^16 values while neighbors sit at extreme/boundary
     * patterns in both words. */
    printf("[3/6] directed lane-crosstalk cases\n");
    {
        static const uint32_t npat[] = { 0x0000, 0xFFFF, 0x7FFF, 0x8000 };
        static const uint32_t npat_y[] = { 0xFFFF, 0x0000, 0x8000, 0x7FFF };
        uint64_t cases0 = total_cases;
        for (unsigned p = 0; p < 4; p++) {
            for (unsigned k = 0; k < 4; k++) {
                uint64_t nx = 0, ny = 0;
                for (unsigned i = 0; i < 4; i++) {
                    if (i == p)
                        continue;
                    nx |= (uint64_t)npat[k] << (16 * i);
                    ny |= (uint64_t)npat_y[k] << (16 * i);
                }
                for (uint32_t v = 0; v < 0x10000u; v++) {
                    uint64_t x = nx | ((uint64_t)v << (16 * p));
                    /* y's test lane: extremes, the same value,
                     * and the top-bit-flipped value, to stress
                     * the (hx, hy) case split in every lane. */
                    uint32_t ws[] = { 0x0000u, 0xFFFFu, v, v ^ 0x8000u };
                    for (unsigned wi = 0; wi < 4; wi++) {
                        uint64_t y = ny | ((uint64_t)ws[wi] << (16 * p));
                        check2(x, y);
                    }
                }
            }
        }
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches);
    }

    /* [4] horizontal max: directed words plus 10M splitmix64 words. */
    printf("[4/6] horizontal max: directed + random 64-bit words\n");
    {
        uint64_t cases0 = total_cases;
        /* Directed: max planted in each lane position, ties,
         * extremes, monotone patterns. */
        static const uint64_t dw[] = {
            0x000100010001FFFFULL, 0x00010001FFFF0001ULL,
            0x0001FFFF00010001ULL, 0xFFFF000100010001ULL,
            0x8000800080008000ULL, 0x7FFF7FFF7FFF7FFFULL,
            0x0000000000000001ULL, 0x0001000000000000ULL,
            0xFFFF000000000000ULL, 0x0000FFFF00000000ULL,
            0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL,
            0x80017FFE80027FFDULL, 0x0000FFFF80007FFFULL,
        };
        for (unsigned i = 0; i < sizeof(dw) / sizeof(dw[0]); i++)
            checkh(dw[i]);
        uint64_t st = UINT64_C(0x123456789ABCDEF0);
        for (uint64_t i = 0; i < 10000000u; i++)
            checkh(splitmix64(&st));
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(total_cases - cases0),
               (unsigned long long)total_mismatches);
    }

    printf("[5/6] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan+UBSan\n");

    /* [6] throughput.  Inputs are pre-generated so the PRNG is
     * outside the timed loop; one extra pass times PRNG+max. */
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

        double best_h = 1e9;
        for (unsigned pass = 0; pass < 5; pass++) {
            uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i < NB; i++)
                    sink += swar_max_u16x4(buf[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)NB * REPS);
            printf("  hmax pass %u: %.3f ns/value (%.3f M values/s) sink=%llx\n",
                   pass, per * 1e9, 1.0 / per / 1e6,
                   (unsigned long long)sink);
            fnv = fnv1a_step(fnv, sink);
            if (per < best_h)
                best_h = per;
        }
        printf("  hmax throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
               best_h * 1e9, 1.0 / best_h / 1e6);

        /* Pairwise core throughput, same method. */
        double best = 1e9;
        for (unsigned pass = 0; pass < 5; pass++) {
            uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i + 1 < NB; i += 2)
                    sink += swar_max2_u16x4(buf[i], buf[i + 1]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double dt = (t1.tv_sec - t0.tv_sec)
                      + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double per = dt / ((uint64_t)(NB / 2) * REPS);
            printf("  max2 pass %u: %.3f ns/value (%.3f M values/s) sink=%llx\n",
                   pass, per * 1e9, 1.0 / per / 1e6,
                   (unsigned long long)sink);
            if (per < best)
                best = per;
        }
        printf("  max2 throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
               best * 1e9, 1.0 / best / 1e6);

        /* With the PRNG inside the timed loop, for comparison. */
        {
            uint64_t sink = 0;
            struct timespec t0, t1;
            st = UINT64_C(0x123456789ABCDEF0);
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned r = 0; r < REPS; r++)
                for (size_t i = 0; i < NB; i++)
                    sink += swar_max_u16x4(splitmix64(&st));
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
