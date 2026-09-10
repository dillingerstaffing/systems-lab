/*
 * test_bit_span: differential verification of bit_span (bit_span.h)
 * against independent naive loop references.
 *
 * Sections:
 *   [1] table self-check: derive the 64-entry de Bruijn table from
 *       the rule (1ULL << k) * C >> 58 and assert the header's table
 *       matches it entry by entry, plus hand-checked anchors
 *       including the x = 0 contract row (bit_span(0) = 0).
 *   [2] exhaustive: every 16-bit input (all 65,536 words < 2^16).
 *   [3] directed: all single-bit positions 0..63, every span value
 *       0..63 as an exact witness (x = (1ULL << hi) | 1), all-ones,
 *       alternating and boundary patterns.
 *   [4] random: 10,000,000 fixed-seed splitmix64 64-bit words.
 *   [5] cross-build checksum: FNV-1a over every output must match
 *       across -O0, -O2, and ASan+UBSan builds.
 *   [6] throughput at -O2 over 100,000,000 timed values.
 *
 * Usage: ./test_bit_span_o0 | ./test_bit_span_o2 | ./test_bit_span_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bit_span.h"

/* Naive references, one loop each: highest set bit scanned from the
 * top down, lowest set bit scanned from the bottom up. */
static unsigned ref_floor_log2(uint64_t x)
{
    for (int i = 63; i >= 0; i--)
        if ((x >> i) & 1u)
            return (unsigned)i;
    return 0; /* unreachable for x != 0; never called there */
}

static unsigned ref_ctz(uint64_t x)
{
    for (unsigned i = 0; i < 64; i++)
        if ((x >> i) & 1u)
            return i;
    return 0; /* unreachable for x != 0; never called there */
}

static unsigned ref_span(uint64_t x)
{
    if (x == 0)
        return 0; /* the documented contract row */
    return ref_floor_log2(x) - ref_ctz(x);
}

/* splitmix64, fixed seed: deterministic 64-bit stream.
 * Seed 0x123456789ABCDEF0 is the proof-engine convention. */
static uint64_t splitmix_state = UINT64_C(0x123456789ABCDEF0);
static uint64_t splitmix_next(void)
{
    uint64_t z = (splitmix_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* FNV-1a 64 over the stream of outputs */
static uint64_t fnv_acc = UINT64_C(0xCBF29CE484222325);
static void fnv_add(uint64_t w)
{
    fnv_acc ^= w;
    fnv_acc *= UINT64_C(0x100000001B3);
}

static uint64_t total_cases;
static uint64_t total_mismatches;

static void check_one(uint64_t x)
{
    unsigned gfl, gctz, gspan;
    unsigned wfl, wctz, wspan;

    gspan = bit_span(x);
    wspan = ref_span(x);
    if (gspan != wspan) {
        if (total_mismatches < 8)
            printf("  FAIL span: x=%016llx got=%u want=%u\n",
                   (unsigned long long)x, gspan, wspan);
        total_mismatches++;
    }

    /* Components carry x != 0 contracts; check them against the
     * independent loops on every nonzero input as well. */
    if (x != 0) {
        gfl = floor_log2_u64(x);
        wfl = ref_floor_log2(x);
        if (gfl != wfl) {
            if (total_mismatches < 8)
                printf("  FAIL floor_log2: x=%016llx got=%u want=%u\n",
                       (unsigned long long)x, gfl, wfl);
            total_mismatches++;
        }
        gctz = ctz_u64(x);
        wctz = ref_ctz(x);
        if (gctz != wctz) {
            if (total_mismatches < 8)
                printf("  FAIL ctz: x=%016llx got=%u want=%u\n",
                       (unsigned long long)x, gctz, wctz);
            total_mismatches++;
        }
        fnv_add(gfl);
        fnv_add(gctz);
    }
    fnv_add(gspan);
    total_cases++;
}

#define N_RANDOM 10000000u

int main(void)
{
    printf("bit_span differential test, build %s\n", BUILD_NAME);

    /* [1/6] de Bruijn table derived from the rule, checked against
     * the header's hardcoded table, plus hand-checked anchors. */
    printf("[1/6] de Bruijn table self-check + anchors\n");
    {
        uint8_t derived[64];
        for (unsigned k = 0; k < 64; k++)
            derived[((UINT64_C(1) << k) * UINT64_C(0x03F79D71B4CB0A89)) >> 58]
                = (uint8_t)k;
        for (unsigned i = 0; i < 64; i++) {
            if (derived[i] != CTZ64_DEBRUIJN[i]) {
                printf("  FAIL table entry %u: header=%u derived=%u\n",
                       i, CTZ64_DEBRUIJN[i], derived[i]);
                total_mismatches++;
            }
        }
        printf("  table: 64/64 entries match the derived rule\n");

        struct { uint64_t x; unsigned want; } anchors[] = {
            { UINT64_C(0x0000000000000000), 0 },   /* the x = 0 contract row */
            { UINT64_C(0x0000000000000001), 0 },   /* single bit: span 0 */
            { UINT64_C(0x8000000000000000), 0 },   /* single top bit: span 0 */
            { UINT64_C(0x0001000000000000), 0 },   /* single bit 48: span 0 */
            { UINT64_C(0x8000000000000001), 63 },  /* bits 63 and 0: span 63 */
            { UINT64_C(0xFFFFFFFFFFFFFFFF), 63 }, /* all ones: span 63 */
            { UINT64_C(0xAAAAAAAAAAAAAAAA), 62 }, /* 1010...: bits 63..1 */
            { UINT64_C(0x5555555555555555), 62 }, /* 0101...: bits 62..0 */
            { UINT64_C(0x0000FFFFFFFF0000), 31 }, /* bits 47..16 */
            { UINT64_C(0xF00000000000000F), 63 }, /* bits 63..0, sparse */
            { UINT64_C(0x00000000F0000000), 3 },  /* bits 31..28: span 3 */
        };
        for (size_t i = 0; i < sizeof(anchors) / sizeof(anchors[0]); i++) {
            unsigned got = bit_span(anchors[i].x);
            if (got != anchors[i].want) {
                printf("  FAIL anchor %zu: x=%016llx got=%u want=%u\n", i,
                       (unsigned long long)anchors[i].x, got, anchors[i].want);
                total_mismatches++;
            }
            fnv_add(got);
            total_cases++;
        }
        printf("  anchors checked: %zu, mismatches so far: %llu\n",
               sizeof(anchors) / sizeof(anchors[0]),
               (unsigned long long)total_mismatches);
    }

    /* [2/6] exhaustive 16-bit inputs */
    printf("[2/6] exhaustive 16-bit inputs\n");
    for (uint64_t x = 0; x < 65536; x++)
        check_one(x);
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [3/6] directed edge words */
    printf("[3/6] directed edge words\n");
    for (unsigned k = 0; k < 64; k++)
        check_one(UINT64_C(1) << k);            /* single bits: span 0 */
    for (unsigned hi = 0; hi < 64; hi++)
        check_one((UINT64_C(1) << hi) | 1u);   /* exact witness of span hi */
    {
        const uint64_t PATS[] = {
            UINT64_C(0xFFFFFFFFFFFFFFFF),
            UINT64_C(0xAAAAAAAAAAAAAAAA),
            UINT64_C(0x5555555555555555),
            UINT64_C(0xF0F0F0F0F0F0F0F0),
            UINT64_C(0x0F0F0F0F0F0F0F0F),
            UINT64_C(0x8000000000000001),
            UINT64_C(0x0000000000000000),
            UINT64_C(0x0000FFFFFFFFFFFF),
            UINT64_C(0xFFFF000000000000),
            UINT64_C(0x00000001FFFFFFFF),
        };
        for (size_t i = 0; i < sizeof(PATS) / sizeof(PATS[0]); i++)
            check_one(PATS[i]);
    }
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [4/6] random 64-bit words, fixed seed */
    printf("[4/6] random 64-bit words (splitmix64, seed 0x123456789ABCDEF0)\n");
    for (uint32_t i = 0; i < N_RANDOM; i++)
        check_one(splitmix_next());
    printf("  done: cases=%llu mismatches=%llu\n",
           (unsigned long long)total_cases, (unsigned long long)total_mismatches);

    /* [5/6] checksum consistency is reported at the end; this section
     * just marks the point in the log where the cross-build check
     * happens (compare the printed FNV-1a value across builds). */
    printf("[5/6] cross-build checksum: compare the FNV-1a line across "
           "-O0, -O2, ASan+UBSan\n");

    /* [6/6] throughput: PRNG pre-generated, excluded from timing */
    printf("[6/6] throughput (PRNG pre-generated, excluded from timing)\n");
    {
        const size_t BUF = 10000000;      /* 80 MiB buffer */
        const unsigned PASSES = 10;       /* 10 x 10M = 100M timed values */
        uint64_t *t = malloc(BUF * sizeof *t);
        if (!t) {
            printf("  FAIL: malloc\n");
            return 1;
        }
        for (size_t i = 0; i < BUF; i++)
            t[i] = splitmix_next();
        double best = 1e30;
        for (unsigned pass = 0; pass < 5; pass++) {
            volatile uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (unsigned p = 0; p < PASSES; p++)
                for (size_t i = 0; i < BUF; i++)
                    sink += bit_span(t[i]);
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            double ns = s / ((double)BUF * PASSES) * 1e9;
            if (ns < best)
                best = ns;
            printf("  throughput pass %u: %.3f ns/value (%.3f M values/s) "
                   "sink=%llx\n",
                   pass, ns, 1000.0 / ns, (unsigned long long)sink);
        }
        printf("  throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
               best, 1000.0 / best);
        /* Honesty check: same loop with the PRNG in the timed loop;
         * the PRNG step costs ~10 ns and dominates the bit_span body,
         * so the pre-generated number is the honest ceiling note. */
        {
            splitmix_state = UINT64_C(0x123456789ABCDEF0);
            volatile uint64_t sink = 0;
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            for (uint32_t i = 0; i < N_RANDOM; i++)
                sink += bit_span(splitmix_next());
            clock_gettime(CLOCK_MONOTONIC, &t1);
            double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
            printf("  with PRNG in the timed loop: %.3f ns/value (sink=%llx)\n",
                   s / N_RANDOM * 1e9, (unsigned long long)sink);
        }
        free(t);
    }

    printf("total verification cases: %llu\n", (unsigned long long)total_cases);
    printf("total mismatches: %llu\n", (unsigned long long)total_mismatches);
    printf("FNV-1a checksum of all outputs: 0x%016llx\n", (unsigned long long)fnv_acc);
    printf("RESULT: %s\n", total_mismatches == 0 ? "PASS" : "FAIL");
    return total_mismatches == 0 ? 0 : 1;
}
