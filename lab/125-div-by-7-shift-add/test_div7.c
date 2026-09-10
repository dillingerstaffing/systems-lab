#include <stdint.h>
#include <stdio.h>

#include "div7.h"

/* Differential test: udiv7_shiftadd against the C / operator.
 *
 *  - targeted edge cases (0, small values, values around 2^24, values
 *    around UINT32_MAX)
 *  - exhaustive: every 24-bit input, 0 .. 2^24 - 1
 *  - 10,000,000 fixed-seed splitmix64 32-bit values (seed 0x123456789ABCDEF0)
 *
 * Also records the maximum number of correction-loop steps taken (recovered
 * as ref - q_series, which equals the loop's iteration count exactly) and an
 * FNV-1a 64-bit checksum over every result, so builds can be cross-checked.
 */

static uint64_t rng_state = 0x123456789ABCDEF0ull;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 14695981039346656037ull;

static void fnv_mix(uint32_t v)
{
    int b;
    for (b = 0; b < 4; b++) {
        fnv1a ^= (uint64_t)((v >> (8 * b)) & 0xFFu);
        fnv1a *= 1099511628211ull;
    }
}

static uint64_t mismatches;
static uint64_t invariant_hits;
static unsigned max_corr;

static void check_one(uint32_t x)
{
    uint32_t got = udiv7_shiftadd(x);
    uint32_t ref = x / 7u; /* differential reference only; not in div7.h */
    /* the shift-add estimate, same expression as in the implementation */
    uint32_t qs = (x >> 3) + (x >> 6) + (x >> 9) + (x >> 12) + (x >> 15)
                + (x >> 18) + (x >> 21) + (x >> 24) + (x >> 27) + (x >> 30);
    unsigned corr;

    if (qs > ref) { /* must never happen: estimate never exceeds x/7 */
        if (invariant_hits < 8)
            printf("INVARIANT qs>ref: x=%u qs=%u ref=%u\n", x, qs, ref);
        invariant_hits++;
    }
    corr = ref - qs; /* exact number of correction steps the loop takes */
    if (corr > max_corr)
        max_corr = corr;
    if (got != ref) {
        if (mismatches < 8)
            printf("MISMATCH x=%u got=%u ref=%u\n", x, got, ref);
        mismatches++;
    }
    fnv_mix(got);
}

int main(void)
{
    static const uint32_t edges[] = {
        0u, 1u, 6u, 7u, 8u, 13u, 14u, 15u,
        0x00FFFFFEu, 0x00FFFFFFu, 0x01000000u, 0x01000001u,
        0x7FFFFFFFu, 0x80000000u,
        0xFFFFFFF8u, 0xFFFFFFF9u, 0xFFFFFFFAu, 0xFFFFFFFBu,
        0xFFFFFFFCu, 0xFFFFFFFDu, 0xFFFFFFFEu, 0xFFFFFFFFu,
    };
    size_t i;
    uint64_t n;

    for (i = 0; i < sizeof edges / sizeof edges[0]; i++)
        check_one(edges[i]);
    printf("edge cases done: %u cases\n", (unsigned)(sizeof edges / sizeof edges[0]));

    for (n = 0; n < (1ull << 24); n++)
        check_one((uint32_t)n);
    printf("exhaustive 24-bit done: 16777216 values\n");

    rng_state = 0x123456789ABCDEF0ull;
    for (n = 0; n < 10000000ull; n++)
        check_one((uint32_t)splitmix64());
    printf("random 32-bit done: 10000000 values, splitmix64 seed 0x123456789ABCDEF0\n");

    printf("total checks          : %llu\n",
           (unsigned long long)((1ull << 24) + 10000000ull + sizeof edges / sizeof edges[0]));
    printf("mismatches            : %llu\n", (unsigned long long)mismatches);
    printf("estimate-above-ref hits: %llu\n", (unsigned long long)invariant_hits);
    printf("max correction steps  : %u\n", max_corr);
    printf("FNV-1a of all results : 0x%016llx\n", (unsigned long long)fnv1a);
    return (mismatches == 0 && invariant_hits == 0) ? 0 : 1;
}
