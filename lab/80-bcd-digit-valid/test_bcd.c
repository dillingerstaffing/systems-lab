#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bcd.h"

/* Independent per-nibble loop reference: ground truth. */
static uint8_t ref_invalid_mask(uint16_t w)
{
    uint8_t m = 0;
    for (int i = 0; i < 4; i++) {
        unsigned n = (unsigned)((w >> (4 * i)) & 0xFu);
        if (n >= 10)
            m |= (uint8_t)(1u << i);
    }
    return m;
}

/*
 * The naive form, w + 0x6666 with carry-out bits read per nibble.
 * Included only to show the aliasing hazard is real: a carry generated
 * at a low invalid nibble propagates through higher valid nibbles and
 * falsely flags them. It is NOT part of the shipped construction.
 */
static uint8_t naive_invalid_mask(uint16_t w)
{
    uint32_t t = (uint32_t)w + 0x6666u;
    uint8_t m = 0;
    /* carry out of nibble i = bit 4i+4 of t xor addend bits there (all 1 in
       0x6666 except the gaps... simpler: recompute with the carry identity) */
    uint32_t a = w, b = 0x6666u;
    for (int i = 0; i < 4; i++) {
        int k = 4 * i + 4;
        unsigned ck = ((t >> k) & 1u) ^ ((a >> k) & 1u) ^ ((b >> k) & 1u);
        if (ck)
            m |= (uint8_t)(1u << i);
    }
    return m;
}

/* splitmix64: deterministic stream of benchmark inputs (BENCH only). */
#ifdef BENCH
static uint64_t rng_state = 0x123456789ABCDEF0ULL;
static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}
#endif

static uint64_t fnv1a = 1469598103934665603ULL;
static void fnv(uint64_t v)
{
    fnv1a ^= v;
    fnv1a *= 1099511628211ULL;
}

int main(void)
{
    long long checks = 0;
    long long mismatches = 0;
    long long naive_wrong = 0;

    /* 1. Exhaustive over all 65,536 16-bit inputs, differential-checked
       against the per-nibble reference. */
    for (uint32_t x = 0; x < 65536u; x++) {
        uint16_t w = (uint16_t)x;
        if (bcd_invalid_mask(w) != ref_invalid_mask(w)) {
            printf("MISMATCH w=%04x got=%x want=%x\n",
                   w, bcd_invalid_mask(w), ref_invalid_mask(w));
            mismatches++;
        }
        if (naive_invalid_mask(w) != ref_invalid_mask(w))
            naive_wrong++;
        fnv((uint64_t)bcd_invalid_mask(w));
        checks++;
    }

    /* 2. Directed rows that alias under the naive form, plus boundary
       values, each checked against the reference. */
    static const uint16_t directed[] = {
        0x9A00, 0x0A00, 0x9A9A, 0xF900, 0x0900, 0x000A,
        0xA000, 0x00A0, 0x0A90, 0xFFFF, 0x0000, 0x9999,
        0xAAAA, 0xF000, 0x000F, 0x9909, 0x0990, 0xA9A9
    };
    for (size_t i = 0; i < sizeof(directed) / sizeof(directed[0]); i++) {
        uint16_t w = directed[i];
        uint8_t got = bcd_invalid_mask(w);
        uint8_t want = ref_invalid_mask(w);
        uint8_t naive = naive_invalid_mask(w);
        printf("directed w=%04x mask=%x ref=%x naive=%x%s\n",
               w, got, want, naive, (got == want) ? "" : " MISMATCH");
        if (got != want)
            mismatches++;
        fnv((uint64_t)got);
        checks++;
    }

    printf("checks=%lld mismatches=%lld naive_form_wrong=%lld fnv1a=%016llx\n",
           checks, mismatches, naive_wrong, (unsigned long long)fnv1a);

#ifdef BENCH
    /*
     * Throughput at -O2: 1M pre-filled splitmix64 values, 25 passes,
     * best of 5. Prefilling separates the RNG cost from the measured
     * function; the timed loop is only the mask computation.
     */
    static uint16_t vals[1 << 20];
    rng_state = 0x123456789ABCDEF0ULL;
    for (size_t i = 0; i < sizeof(vals) / sizeof(vals[0]); i++)
        vals[i] = (uint16_t)splitmix64();
    const long long total = (long long)(sizeof(vals) / sizeof(vals[0])) * 25;
    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint8_t sink = 0;
        for (int pass = 0; pass < 25; pass++)
            for (size_t i = 0; i < sizeof(vals) / sizeof(vals[0]); i++)
                sink ^= bcd_invalid_mask(vals[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    printf("bench: %.2f ns/value (%.1f Mvalues/s over %lld timed values, best of 5)\n",
           best / (double)total, (double)total / (best / 1e3), total);
#endif

    return mismatches == 0 ? 0 : 1;
}
