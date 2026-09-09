/* test_absdiff.c — differential verification of badiff64.
 *
 * Coverage:
 *   1. Exhaustive over all 2^16 x 2^16 = 4,294,967,296 pairs of int16_t
 *      values, sign-extended into int64_t, checked against llabs(d).
 *      Every difference fits in int64_t and is never INT64_MIN, so the
 *      whole phase is inside the contract.
 *   2. Directed 9x9 = 81 edge pairs from {INT64_MIN, INT64_MIN+1,
 *      -2^60, -1, 0, 1, 2^60, INT64_MAX-1, INT64_MAX}. The exact
 *      difference is computed in __int128; pairs whose true difference
 *      does not fit int64_t, or equals INT64_MIN, are skipped and
 *      counted (the (INT64_MIN, 0) pair is the documented exclusion).
 *   3. 1,000,000 fixed-seed splitmix64 random pairs with each operand
 *      in [-2^60, 2^60), so every difference fits in int64_t and is
 *      never INT64_MIN by construction; checked against llabs(d).
 *   4. Throughput: timed loop of 200,000,000 pairs, ns/pair, with an
 *      XOR/ADD checksum sink so the three builds can be compared.
 *
 * The PRNG is splitmix64 with a fixed 64-bit seed (0x243F6A8885A308D3,
 * the fractional part of pi); every run below is reproducible.
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> /* llabs */
#include <time.h>

#include "absdiff.h"

/* Fixed seed, documented: fractional digits of pi. */
static uint64_t rng_state = 0x243F6A8885A308D3ull;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint64_t total_pairs;
static uint64_t mismatches;
static uint64_t skipped_pairs; /* phase 2 only: outside the contract */

/* Oracle: exact difference in __int128; excluded=1 when the pair is
 * outside badiff64's contract (difference overflows int64_t, or the
 * documented INT64_MIN exclusion). */
static uint64_t oracle_absdiff(int64_t a, int64_t b, int *excluded)
{
    __int128 d = (__int128)a - (__int128)b;
    if (d < (__int128)INT64_MIN + 1 || d > (__int128)INT64_MAX) {
        *excluded = 1;
        return 0;
    }
    *excluded = 0;
    return (uint64_t)llabs((int64_t)d);
}

static void check_pair(int64_t a, int64_t b)
{
    int excluded = 0;
    uint64_t ref = oracle_absdiff(a, b, &excluded);
    if (excluded) {
        skipped_pairs++;
        return;
    }
    uint64_t got = badiff64(a, b);
    total_pairs++;
    if (got != ref) {
        mismatches++;
        if (mismatches <= 10)
            printf("MISMATCH a=%lld b=%lld badiff=%llu ref=%llu\n",
                   (long long)a, (long long)b,
                   (unsigned long long)got, (unsigned long long)ref);
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
    /* Phase 1: exhaustive 16-bit pairs, sign-extended to int64_t. */
    double t0 = now_s();
    for (int64_t a16 = -32768; a16 <= 32767; a16++) {
        for (int64_t b16 = -32768; b16 <= 32767; b16++)
            check_pair((int64_t)(int16_t)a16, (int64_t)(int16_t)b16);
        if ((a16 & 8191) == 0)
            printf("exhaustive 16-bit sweep: a=%lld of 32767, pairs=%llu, mismatches=%llu\n",
                   (long long)a16, (unsigned long long)total_pairs,
                   (unsigned long long)mismatches);
    }
    double t1 = now_s();
    printf("phase 1 (exhaustive 16-bit pairs): %llu pairs, %llu mismatches, %.1f s\n",
           (unsigned long long)total_pairs, (unsigned long long)mismatches, t1 - t0);

    /* Phase 2: directed 64-bit edges, including the INT64_MIN exclusion. */
    static const int64_t edge[] = {
        INT64_MIN, INT64_MIN + 1,
        -(INT64_C(1) << 60), -1, 0, 1,
        (INT64_C(1) << 60), INT64_MAX - 1, INT64_MAX
    };
    uint64_t phase2_skipped = 0;
    for (unsigned i = 0; i < sizeof(edge) / sizeof(edge[0]); i++)
        for (unsigned j = 0; j < sizeof(edge) / sizeof(edge[0]); j++)
            check_pair(edge[i], edge[j]);
    phase2_skipped = skipped_pairs;
    printf("phase 2 (directed 64-bit edges): %u pairs checked, %llu skipped (outside contract), %llu mismatches\n",
           81u, (unsigned long long)phase2_skipped,
           (unsigned long long)mismatches);

    /* Phase 3: 1M fixed-seed random pairs, operands in [-2^60, 2^60).
     * By construction every difference lies in (-2^61, 2^61): it always
     * fits int64_t and can never be INT64_MIN, so nothing is skipped. */
    uint64_t before = total_pairs;
    for (uint64_t n = 0; n < 1000000; n++) {
        int64_t a = (int64_t)(splitmix64() >> 3) - (INT64_C(1) << 60);
        int64_t b = (int64_t)(splitmix64() >> 3) - (INT64_C(1) << 60);
        assert(a >= -(INT64_C(1) << 60) && a < (INT64_C(1) << 60));
        assert(b >= -(INT64_C(1) << 60) && b < (INT64_C(1) << 60));
        check_pair(a, b);
    }
    printf("phase 3 (1M random 64-bit pairs): %llu pairs, %llu mismatches\n",
           (unsigned long long)(total_pairs - before),
           (unsigned long long)mismatches);
    if (skipped_pairs != phase2_skipped) {
        printf("UNEXPECTED: phase 1/3 skipped pairs (contract violation in generator)\n");
        return 1;
    }

    printf("correctness: %llu differential checks, %llu mismatches\n",
           (unsigned long long)total_pairs, (unsigned long long)mismatches);

    /* Phase 4: throughput. Deterministic LCG stream, checksum sink. */
    uint64_t sink = 0;
    uint64_t lcg = 0x123456789ABCDEF0ull;
    double tb0 = now_s();
    for (uint64_t n = 0; n < 200000000; n++) {
        lcg = lcg * 6364136223846793005ull + 1442695040888963407ull;
        int64_t a = (int64_t)(lcg >> 3) - (INT64_C(1) << 60);
        lcg = lcg * 6364136223846793005ull + 1442695040888963407ull;
        int64_t b = (int64_t)(lcg >> 3) - (INT64_C(1) << 60);
        uint64_t r = badiff64(a, b);
        sink ^= r + 0x9E3779B97F4A7C15ull + (sink << 6) + (sink >> 2);
    }
    double tb1 = now_s();
    printf("throughput: %.3f ns/pair over 200000000 pairs (sink=0x%llx)\n",
           (tb1 - tb0) * 1e9 / 200000000.0, (unsigned long long)sink);

    if (mismatches == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("FAILURES: %llu mismatches\n", (unsigned long long)mismatches);
    return 1;
}
