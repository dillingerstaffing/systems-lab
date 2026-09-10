/* test_avg_round_half.c - differential test of avg_rhu_u64.
 *
 * Reference oracle: the exact sum in unsigned __int128, then
 * round-half-up, i.e. floor((s + 1) / 2). The oracle lives in the test
 * only; the implementation in avg_round_half.h never uses __int128.
 *
 * Phase 1: exhaustive over all 2^32 pairs of uint16_t operands.
 * Phase 2: directed tie rows, odd-sum cases where the exact average
 *          ends in .5, printed as a table to pin the tie behavior.
 * Phase 3: 10,000,000 fixed-seed splitmix64 random 64-bit pairs.
 *
 * Every result is folded into a 64-bit FNV-1a checksum (byte at a
 * time, low byte first) so builds can be compared bit for bit.
 *
 * With -DBENCH the correctness phases are skipped and throughput is
 * measured instead.
 */
#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "avg_round_half.h"

/* Fixed seed 0x123456789ABCDEF0: every run is reproducible. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Exact round-half-up of (a + b) / 2, computed in __int128 so the sum
 * itself is exact. floor((s + 1) / 2) rounds an odd s up, an even s
 * exactly. */
#ifndef BENCH
static uint64_t oracle(uint64_t a, uint64_t b)
{
    return (uint64_t)(((unsigned __int128)a + b + 1u) / 2u);
}

static uint64_t fnv1a = 0xcbf29ce484222325ULL;

static void fnv_fold(uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        fnv1a ^= (v >> (8 * i)) & 0xFFu;
        fnv1a *= 0x100000001B3ULL;
    }
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}
#endif

int main(void)
{
#ifdef BENCH
    /* Throughput: 25M values at -O2, best-of-5. */
    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        rng_state = 0x123456789ABCDEF0ULL + (uint64_t)rep;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint64_t sink = 0;
        for (int i = 0; i < 25000000; i++)
            sink ^= avg_rhu_u64(rng_next(), rng_next());
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (double)(t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    printf("bench: %.2f ns/value (%.1f Mvalues/s over 25M timed values, "
           "best of 5)\n",
           best / 25000000.0, 25000000.0 / (best / 1e3));
    return 0;
#else
    uint64_t mismatches = 0;
    uint64_t total_pairs = 0;

    /* Phase 1: exhaustive 16-bit pairs. */
    {
        uint64_t pairs = 0;
        double t0 = now_s();
        for (uint32_t a = 0; a <= 0xFFFFu; a++) {
            for (uint32_t b = 0; b <= 0xFFFFu; b++) {
                uint64_t got = avg_rhu_u64((uint64_t)a, (uint64_t)b);
                uint64_t want = oracle((uint64_t)a, (uint64_t)b);
                mismatches += (uint64_t)(got != want);
                fnv_fold(got);
                pairs++;
            }
            if ((a & 0x3FFFu) == 0)
                printf("exhaustive: a=%5u of 65535, pairs=%" PRIu64
                       ", mismatches=%" PRIu64 "\n",
                       a, pairs, mismatches);
        }
        double dt = now_s() - t0;
        total_pairs += pairs;
        printf("phase 1 (exhaustive 16-bit pairs): %" PRIu64 " pairs, %"
               PRIu64 " mismatches, %.1f s (%.2f ns/pair)\n",
               pairs, mismatches, dt, dt * 1e9 / (double)pairs);
    }

    /* Phase 2: directed tie rows. Each prints the tie bit ((a^b)&1),
     * the value produced, and the exact oracle. */
    {
        static const uint64_t ties[][2] = {
            {0x0000000000000000ULL, 0x0000000000000000ULL},
            {0x0000000000000000ULL, 0x0000000000000001ULL},
            {0x0000000000000001ULL, 0x0000000000000000ULL},
            {0x0000000000000001ULL, 0x0000000000000001ULL},
            {0x0000000000000002ULL, 0x0000000000000003ULL},
            {0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL},
            {0xFFFFFFFFFFFFFFFFULL, 0x0000000000000001ULL},
            {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFEULL},
            {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL},
            {0x8000000000000000ULL, 0x8000000000000001ULL},
            {0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL},
            {0xAAAAAAAAAAAAAAAAULL, 0xAAAAAAAAAAAAAAAAULL},
            {0x0000000000000001ULL, 0xFFFFFFFFFFFFFFFFULL},
        };
        uint64_t n = sizeof(ties) / sizeof(ties[0]);
        uint64_t phase_mism = 0;
        printf("phase 2 (tie rows, odd-sum cases round up):\n");
        printf("%20s %20s %4s %20s %20s %s\n", "a", "b", "tie",
               "got", "want", "ok");
        for (uint64_t i = 0; i < n; i++) {
            uint64_t a = ties[i][0];
            uint64_t b = ties[i][1];
            uint64_t got = avg_rhu_u64(a, b);
            uint64_t want = oracle(a, b);
            uint64_t tie = (a ^ b) & 1u;
            int ok = (got == want);
            phase_mism += (uint64_t)(!ok);
            fnv_fold(got);
            printf("%20" PRIu64 " %20" PRIu64 " %4" PRIu64 " %20" PRIu64
                   " %20" PRIu64 " %s\n",
                   a, b, tie, got, want, ok ? "ok" : "MISMATCH");
        }
        mismatches += phase_mism;
        total_pairs += n;
        printf("phase 2: %" PRIu64 " tie rows, %" PRIu64 " mismatches\n",
               n, phase_mism);
    }

    /* Phase 3: 10M fixed-seed random 64-bit pairs. */
    {
        const uint64_t N = 10000000ULL;
        uint64_t phase_mism = 0;
        rng_state = 0x123456789ABCDEF0ULL; /* reset for reproducibility */
        double t0 = now_s();
        for (uint64_t k = 0; k < N; k++) {
            uint64_t a = rng_next();
            uint64_t b = rng_next();
            uint64_t got = avg_rhu_u64(a, b);
            uint64_t want = oracle(a, b);
            if (got != want) {
                phase_mism++;
                if (phase_mism < 5)
                    printf("MISMATCH random: a=%" PRIu64 " b=%" PRIu64
                           " got=%" PRIu64 " want=%" PRIu64 "\n",
                           a, b, got, want);
            }
            fnv_fold(got);
        }
        double dt = now_s() - t0;
        mismatches += phase_mism;
        total_pairs += N;
        printf("phase 3 (10M random 64-bit pairs): %" PRIu64 " pairs, %"
               PRIu64 " mismatches, %.1f s (%.2f ns/pair)\n",
               N, phase_mism, dt, dt * 1e9 / (double)N);
    }

    printf("correctness: %" PRIu64 " differential checks, %" PRIu64
           " mismatches, fnv1a=%016" PRIx64 "\n",
           total_pairs, mismatches, fnv1a);

    if (mismatches != 0) {
        printf("TESTS FAILED\n");
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
#endif
}
