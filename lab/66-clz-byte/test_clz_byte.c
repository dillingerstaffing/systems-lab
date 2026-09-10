/*
 * test_clz_byte.c - verification of clz64_byte (clz_byte.h).
 *
 * Phases:
 *  1. Table check: every clz8_tab[b] is re-derived by an independent
 *     bit test (scan bits 7 down to 0 for the first 1).
 *  2. Directed checks: all 64 single-bit values plus 10 boundary
 *     patterns (derivations in PROOF.md), each compared against
 *     ref_clz64.
 *  3. Exhaustive differential: all 65,536 16-bit inputs, each compared
 *     against ref_clz64.
 *  4. Random differential: 1,000,000 splitmix64 64-bit values with the
 *     fixed seed 0x123456789ABCDEF0, each compared against ref_clz64.
 *  5. Throughput: timed passes over the random buffer at this build's
 *     optimization level; the accumulated sink is printed so the loop
 *     cannot be discarded.
 *
 * Reference: __builtin_clzll, with 0 mapped to 64 per this module's
 * contract (__builtin_clzll(0) is undefined, so it is never called
 * with 0).
 *
 * Every result byte of phases 1-4 is folded into an FNV-1a checksum
 * printed at the end.  All builds run the same full suite, so an
 * identical checksum across -O0, -O2, and ASan+UBSan builds means
 * every build computed the same answers.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "clz_byte.h"

static uint64_t ref_clz64(uint64_t x)
{
    if (x == 0)
        return 64;
    return (uint64_t)__builtin_clzll(x);
}

/* splitmix64 PRNG; fixed seed makes the random phase reproducible. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 0xCBF29CE484222325ULL;

static void checksum_result(uint64_t r)
{
    for (int i = 0; i < 8; i++) {
        fnv1a ^= (uint8_t)(r >> (8 * i));
        fnv1a *= 0x100000001B3ULL;
    }
}

/* Independent re-derivation of a table entry: first 1 bit from the top. */
static unsigned table_expected(unsigned b)
{
    for (int k = 7; k >= 0; k--) {
        if ((b >> (unsigned)k) & 1U)
            return (unsigned)(7 - k);
    }
    return 8;
}

#define NRAND 1000000
#define BENCH_PASSES 30

static uint64_t buf[NRAND];

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static const uint64_t patterns[] = {
    0x0000000000000000ULL,
    0x0000000000000001ULL,
    0x8000000000000000ULL,
    0xFFFFFFFFFFFFFFFFULL,
    0x7FFFFFFFFFFFFFFFULL,
    0x00F0000000000000ULL,
    0x0000000000000080ULL,
    0x0100000000000000ULL,
    0xAAAAAAAAAAAAAAAAULL,
    0x5555555555555555ULL,
};

int main(void)
{
    unsigned long table_checks = 0, directed = 0, exhaustive = 0, random = 0;
    unsigned long mismatches = 0;

    clz8_build();

    /* Phase 1: table check. */
    for (unsigned b = 0; b < 256; b++) {
        unsigned want = table_expected(b);
        table_checks++;
        checksum_result(clz8_tab[b]);
        if (clz8_tab[b] != (uint8_t)want) {
            if (mismatches < 8)
                printf("TABLE MISMATCH b=0x%02X tab=%u want=%u\n",
                       b, clz8_tab[b], want);
            mismatches++;
        }
    }

    /* Phase 2: directed checks. */
    for (int k = 0; k < 64; k++) {
        uint64_t x = 1ULL << (unsigned)k;
        uint64_t got = clz64_byte(x);
        uint64_t want = ref_clz64(x);
        directed++;
        checksum_result(got);
        if (got != want) {
            if (mismatches < 8)
                printf("DIRECTED MISMATCH x=0x%016llX got=%llu want=%llu\n",
                       (unsigned long long)x,
                       (unsigned long long)got, (unsigned long long)want);
            mismatches++;
        }
    }
    for (unsigned i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        uint64_t x = patterns[i];
        uint64_t got = clz64_byte(x);
        uint64_t want = ref_clz64(x);
        directed++;
        checksum_result(got);
        if (got != want) {
            if (mismatches < 8)
                printf("DIRECTED MISMATCH x=0x%016llX got=%llu want=%llu\n",
                       (unsigned long long)x,
                       (unsigned long long)got, (unsigned long long)want);
            mismatches++;
        }
    }

    /* Phase 3: exhaustive 16-bit differential. */
    for (uint32_t x = 0; x < 65536; x++) {
        uint64_t got = clz64_byte((uint64_t)x);
        uint64_t want = ref_clz64((uint64_t)x);
        exhaustive++;
        checksum_result(got);
        if (got != want) {
            if (mismatches < 8)
                printf("EXHAUSTIVE MISMATCH x=0x%04X got=%llu want=%llu\n",
                       x, (unsigned long long)got, (unsigned long long)want);
            mismatches++;
        }
    }

    /* Phase 4: random differential; also fills the bench buffer. */
    for (unsigned i = 0; i < NRAND; i++) {
        uint64_t x = splitmix64();
        buf[i] = x;
        uint64_t got = clz64_byte(x);
        uint64_t want = ref_clz64(x);
        random++;
        checksum_result(got);
        if (got != want) {
            if (mismatches < 8)
                printf("RANDOM MISMATCH x=0x%016llX got=%llu want=%llu\n",
                       (unsigned long long)x,
                       (unsigned long long)got, (unsigned long long)want);
            mismatches++;
        }
    }

    /* Phase 5: throughput.  One warmup pass, then timed passes. */
    uint64_t sink = 0;
    for (unsigned i = 0; i < NRAND; i++)
        sink += clz64_byte(buf[i]);
    double t0 = now_ns();
    for (int p = 0; p < BENCH_PASSES; p++)
        for (unsigned i = 0; i < NRAND; i++)
            sink += clz64_byte(buf[i]);
    double t1 = now_ns();
    double total_ops = (double)BENCH_PASSES * (double)NRAND;
    double ns_per = (t1 - t0) / total_ops;

    printf("table checks:      %lu\n", table_checks);
    printf("directed checks:   %lu\n", directed);
    printf("exhaustive checks: %lu\n", exhaustive);
    printf("random checks:     %lu\n", random);
    printf("mismatches:        %lu\n", mismatches);
    printf("fnv1a checksum:    0x%016llX\n", (unsigned long long)fnv1a);
    printf("throughput:        %.3f ns/value (%u passes x %u values)\n",
           ns_per, BENCH_PASSES, NRAND);
    printf("sink:              0x%016llX\n", (unsigned long long)sink);

    return mismatches == 0 ? 0 : 1;
}
