/* test_median3.c — differential verification of med3.
 *
 * Coverage:
 *   1. Exhaustive over all 256^3 = 16,777,216 triples of int8_t values,
 *      sign-extended into int64_t, checked against a comparison-based
 *      sort reference (branching, deliberately different construction).
 *   2. Directed 9^3 = 729 edge triples from {INT64_MIN, INT64_MIN+1,
 *      -(2^60), -1, 0, 1, 2^60, INT64_MAX-1, INT64_MAX}, exercising the
 *      extremes of the full 64-bit domain.
 *   3. 5,000,000 fixed-seed splitmix64 random 64-bit triples, seed
 *      0x123456789ABCDEF0, full int64 range on every input.
 *   4. Throughput: timed loop of 100,000,000 triples at -O2, ns/triple,
 *      with an XOR/ADD checksum sink.
 *
 * Every phase feeds each med3 output into an FNV-1a checksum; the final
 * checksum must be identical across the -O0, -O2, and ASan+UBSan builds.
 * The test exits nonzero on any mismatch.
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "median3.h"

/* Fixed seed for the random phase, stated in the task. */
static uint64_t rng_state = 0x123456789ABCDEF0ull;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* Comparison-based reference: 3-element sort with branches, returning
 * the middle element. Deliberately a different construction from the
 * branchless network under test. */
static int64_t ref_median(int64_t a, int64_t b, int64_t c)
{
    int64_t x = a, y = b, z = c, t;
    if (x > y) { t = x; x = y; y = t; }
    if (y > z) { t = y; y = z; z = t; }
    if (x > y) { t = x; x = y; y = t; }
    return y;
}

static uint64_t fnv = 14695981039346656037ull;

static void feed(int64_t v)
{
    uint64_t w = (uint64_t)v;
    for (int i = 0; i < 8; i++) {
        fnv ^= (w >> (8 * i)) & 0xffu;
        fnv *= 1099511628211ull;
    }
}

static uint64_t total_checks;
static uint64_t mismatches;

static void check_triple(int64_t a, int64_t b, int64_t c)
{
    int64_t got = med3(a, b, c);
    int64_t ref = ref_median(a, b, c);
    total_checks++;
    feed(got);
    if (got != ref) {
        mismatches++;
        if (mismatches <= 10)
            printf("MISMATCH a=%lld b=%lld c=%lld med3=%lld ref=%lld\n",
                   (long long)a, (long long)b, (long long)c,
                   (long long)got, (long long)ref);
    }
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(void)
{
    /* Phase 1: exhaustive 8-bit triples, sign-extended to int64_t. */
    double t0 = now_s();
    for (int64_t a8 = -128; a8 <= 127; a8++) {
        for (int64_t b8 = -128; b8 <= 127; b8++) {
            for (int64_t c8 = -128; c8 <= 127; c8++)
                check_triple((int64_t)(int8_t)a8,
                             (int64_t)(int8_t)b8,
                             (int64_t)(int8_t)c8);
        }
        if ((a8 & 63) == 0)
            printf("exhaustive 8-bit sweep: a=%lld of 127, triples=%llu, mismatches=%llu\n",
                   (long long)a8, (unsigned long long)total_checks,
                   (unsigned long long)mismatches);
    }
    double t1 = now_s();
    printf("phase 1 (exhaustive 8-bit triples): %llu triples, %llu mismatches, %.1f s\n",
           (unsigned long long)total_checks,
           (unsigned long long)mismatches, t1 - t0);

    /* Phase 2: directed 64-bit edges, full domain extremes. */
    static const int64_t edge[] = {
        INT64_MIN, INT64_MIN + 1,
        -(INT64_C(1) << 60), -1, 0, 1,
        (INT64_C(1) << 60), INT64_MAX - 1, INT64_MAX
    };
    uint64_t before = total_checks;
    for (unsigned i = 0; i < 9; i++)
        for (unsigned j = 0; j < 9; j++)
            for (unsigned k = 0; k < 9; k++)
                check_triple(edge[i], edge[j], edge[k]);
    printf("phase 2 (directed 64-bit edges): %llu triples, %llu mismatches\n",
           (unsigned long long)(total_checks - before),
           (unsigned long long)mismatches);

    /* Phase 3: 5M fixed-seed random triples, full int64 range. */
    before = total_checks;
    for (uint64_t n = 0; n < 5000000; n++) {
        int64_t a = (int64_t)splitmix64();
        int64_t b = (int64_t)splitmix64();
        int64_t c = (int64_t)splitmix64();
        check_triple(a, b, c);
    }
    double t3 = now_s();
    printf("phase 3 (5M random 64-bit triples): %llu triples, %llu mismatches, %.1f s\n",
           (unsigned long long)(total_checks - before),
           (unsigned long long)mismatches, t3 - t1);

    printf("correctness: %llu differential checks, %llu mismatches\n",
           (unsigned long long)total_checks,
           (unsigned long long)mismatches);
    printf("FNV-1a checksum of all outputs: %llu (0x%llx)\n",
           (unsigned long long)fnv, (unsigned long long)fnv);

    /* Phase 4: throughput. Deterministic LCG stream, checksum sink. */
    uint64_t sink = 0;
    uint64_t lcg = 0x123456789ABCDEF0ull;
    double tb0 = now_s();
    for (uint64_t n = 0; n < 100000000; n++) {
        lcg = lcg * 6364136223846793005ull + 1442695040888963407ull;
        int64_t a = (int64_t)lcg;
        lcg = lcg * 6364136223846793005ull + 1442695040888963407ull;
        int64_t b = (int64_t)lcg;
        lcg = lcg * 6364136223846793005ull + 1442695040888963407ull;
        int64_t c = (int64_t)lcg;
        int64_t r = med3(a, b, c);
        sink ^= (uint64_t)r + 0x9E3779B97F4A7C15ull + (sink << 6) + (sink >> 2);
    }
    double tb1 = now_s();
    printf("throughput: %.3f ns/triple over 100000000 triples (sink=0x%llx)\n",
           (tb1 - tb0) * 1e9 / 100000000.0, (unsigned long long)sink);

    if (mismatches == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("FAILURES: %llu mismatches\n", (unsigned long long)mismatches);
    return 1;
}
