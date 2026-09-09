/* test_minmax.c — differential verification of bmin32/bmax32.
 *
 * Coverage:
 *   1. Exhaustive over all 2^16 x 2^16 = 4,294,967,296 pairs of int16_t
 *      values, sign-extended into int32_t. Every pair is checked against
 *      the ternary-operator reference and against the identities
 *      min(a,b) <= max(a,b) and "each result equals one of the inputs".
 *   2. Directed full-32-bit edges: every pair from
 *      {INT32_MIN, INT32_MIN+1, -2^31+2?, -1, 0, 1, INT32_MAX-1, INT32_MAX},
 *      the region where a 32-bit (a-b)>>31 mask would break.
 *   3. 10,000,000 fixed-seed xorshift32 random full-32-bit pairs.
 *   4. Throughput: timed loop of 200,000,000 pairs at -O2, ns/pair.
 *
 * All randomness is a deterministic xorshift32 with a fixed seed, so
 * every run below is reproducible.
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "minmax.h"

static uint32_t rng_state = 0x9E3779B9u; /* fixed seed */

static uint32_t xorshift32(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static uint64_t total_pairs;
static uint64_t mismatches;

static void check_pair(int32_t a, int32_t b)
{
    int32_t mn = bmin32(a, b);
    int32_t mx = bmax32(a, b);
    int32_t refmn = (a < b) ? a : b;
    int32_t refmx = (a < b) ? b : a;
    total_pairs++;
    if (mn != refmn || mx != refmx || mn > mx ||
        (mn != a && mn != b) || (mx != a && mx != b)) {
        mismatches++;
        if (mismatches <= 10)
            printf("MISMATCH a=%d b=%d bmin=%d bmax=%d refmin=%d refmax=%d\n",
                   a, b, mn, mx, refmn, refmx);
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
    /* Phase 1: exhaustive 16-bit pairs, sign-extended to int32_t. */
    double t0 = now_s();
    for (int32_t a16 = -32768; a16 <= 32767; a16++) {
        for (int32_t b16 = -32768; b16 <= 32767; b16++)
            check_pair((int32_t)(int16_t)a16, (int32_t)(int16_t)b16);
        if ((a16 & 8191) == 0)
            printf("exhaustive 16-bit sweep: a=%d of 32767, pairs=%llu, mismatches=%llu\n",
                   a16, (unsigned long long)total_pairs,
                   (unsigned long long)mismatches);
    }
    double t1 = now_s();
    printf("phase 1 (exhaustive 16-bit pairs): %llu pairs, %llu mismatches, %.1f s\n",
           (unsigned long long)total_pairs, (unsigned long long)mismatches, t1 - t0);

    /* Phase 2: directed 32-bit edges where a 32-bit mask would break. */
    {
        static const int32_t edges[] = {
            INT32_MIN, INT32_MIN + 1, -1, 0, 1, INT32_MAX - 1, INT32_MAX
        };
        uint64_t before = total_pairs;
        for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++)
            for (unsigned j = 0; j < sizeof(edges) / sizeof(edges[0]); j++)
                check_pair(edges[i], edges[j]);
        printf("phase 2 (directed 32-bit edges): %llu pairs, %llu mismatches\n",
               (unsigned long long)(total_pairs - before),
               (unsigned long long)mismatches);
    }

    /* Phase 3: 10M fixed-seed random full-32-bit pairs. */
    {
        uint64_t before = total_pairs;
        for (uint64_t i = 0; i < 10000000u; i++)
            check_pair((int32_t)xorshift32(), (int32_t)xorshift32());
        printf("phase 3 (10M random 32-bit pairs): %llu pairs, %llu mismatches\n",
               (unsigned long long)(total_pairs - before),
               (unsigned long long)mismatches);
    }

    printf("correctness: %llu differential checks, %llu mismatches\n",
           (unsigned long long)total_pairs, (unsigned long long)mismatches);

    /* Phase 4: throughput at -O2. */
    {
        const uint64_t N = 200000000u;
        volatile uint32_t sink = 0; /* defeats dead-code elimination */
        uint32_t x = 0x12345678u, y = 0x9ABCDEF0u;
        double s0 = now_s();
        for (uint64_t i = 0; i < N; i++) {
            x = x * 1664525u + 1013904223u; /* cheap LCG, feeds both inputs */
            y = y * 22695477u + 1u;
            sink ^= (uint32_t)bmin32((int32_t)x, (int32_t)y);
            sink ^= (uint32_t)bmax32((int32_t)x, (int32_t)y);
        }
        double s1 = now_s();
        double ns = (s1 - s0) * 1e9 / (double)N;
        printf("throughput: %.3f ns/pair over %llu pairs (sink=0x%08x)\n",
               ns, (unsigned long long)N, sink);
    }

    if (mismatches == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("FAILURES: %llu\n", (unsigned long long)mismatches);
    return 1;
}
