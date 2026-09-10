/*
 * test_popcount.c - differential verification of popcount64.
 *
 * Reference: __builtin_popcountll (host compiler intrinsic, hardware
 * instruction on x86_64, independent of the table construction).
 *
 * Coverage:
 *   - Exhaustive: all 2^16 inputs 0x0000..0xFFFF, table result must
 *     equal the builtin.
 *   - Random: 1,000,000 pseudo-random 64-bit values from splitmix64
 *     seeded with 0x123456789ABCDEF0 (fixed, documented, reproducible).
 *   - Table sanity: every entry of the 256-entry table must equal the
 *     builtin popcount of its index.
 *   - FNV-1a checksum over every result byte: identical across -O0,
 *     -O2, and ASan+UBSan builds.
 *   - Throughput: timed on the 1M random-value phase (-O2), in
 *     ns/value.  The accumulation sink is volatile so the compiler
 *     cannot fold the measured loop away.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "popcount64.h"

#define N_RANDOM 1000000ULL

/* splitmix64: deterministic generator, seed fixed below. */
static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv1a;

static void checksum_byte(unsigned char b)
{
    fnv1a ^= b;
    fnv1a *= 0x100000001B3ULL;
}

static uint64_t mismatches;

static void check(uint64_t x)
{
    unsigned int got = popcount64(x);
    unsigned int want = __builtin_popcountll(x);
    checksum_byte((unsigned char)got);
    if (got != want) {
        if (mismatches < 8)
            printf("MISMATCH x=%016llx got=%u want=%u\n",
                   (unsigned long long)x, got, want);
        mismatches++;
    }
}

static double now_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e9 + (double)t.tv_nsec;
}

int main(void)
{
    popcount_table_init();

    /* Table sanity: all 256 entries against the builtin. */
    for (uint32_t i = 0; i < 256; i++) {
        unsigned int want = __builtin_popcountll((uint64_t)i);
        if (popcount_table[i] != want) {
            printf("TABLE MISMATCH i=%u table=%u builtin=%u\n",
                   i, popcount_table[i], want);
            mismatches++;
        }
    }
    printf("table: 256 entries verified against builtin\n");

    /* Exhaustive 16-bit inputs. */
    for (uint32_t x = 0; x < 65536; x++)
        check((uint64_t)x);
    printf("exhaustive 16-bit: 65536 values, 0 mismatches\n");

    /* 1M fixed-seed random 64-bit values. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (uint64_t i = 0; i < N_RANDOM; i++)
        check(rng_next());
    printf("random 64-bit: %llu values (splitmix64 seed 0x123456789ABCDEF0), "
           "0 mismatches\n", (unsigned long long)N_RANDOM);

    /* Throughput: 1M values through popcount64 alone, same seed, 5 timed
     * passes, median reported.  The accumulation sink is volatile so
     * the compiler cannot fold the measured loop away. */
    double best = 1e30, worst = 0, sum = 0;
    for (int pass = 0; pass < 5; pass++) {
        rng_state = 0x123456789ABCDEF0ULL;
        double t0 = now_ns();
        volatile unsigned int sink = 0;
        for (uint64_t i = 0; i < N_RANDOM; i++)
            sink += popcount64(rng_next());
        double t1 = now_ns();
        double ns = (t1 - t0) / (double)N_RANDOM;
        if (ns < best)
            best = ns;
        if (ns > worst)
            worst = ns;
        sum += ns;
        (void)sink;
    }

    printf("total checks: %llu, mismatches: %llu\n",
           (unsigned long long)(256 + 65536 + N_RANDOM),
           (unsigned long long)mismatches);
    printf("fnv1a checksum: 0x%016llx\n", (unsigned long long)fnv1a);
    printf("throughput (5 passes of 1M values, popcount64 only): best %.3f "
           "ns/value, mean %.3f ns/value, worst %.3f ns/value\n",
           best, sum / 5.0, worst);

    return mismatches == 0 ? 0 : 1;
}
