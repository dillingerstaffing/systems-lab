/* test_avg.c - differential test of avg_u64 against an exact reference.
 *
 * Reference oracle: the true sum computed in unsigned __int128, then
 * halved, i.e. the exact floor((a + b) / 2). Any pair where avg_u64
 * differs from the oracle is a mismatch.
 *
 * Phase 1 (only when any argv is given): exhaustive over all 2^32 pairs
 * of uint16_t operands.
 * Phase 2: directed edge pairs drawn from across the full uint64_t range.
 * Phase 3: 10,000,000 fixed-seed splitmix64 random 64-bit pairs.
 */
#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "avg.h"

/* Fixed seed 0x243F6A8885A308D3 (fractional digits of pi): every run is
 * reproducible. */
static uint64_t rng_state = 0x243F6A8885A308D3ULL;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t oracle(uint64_t a, uint64_t b)
{
    return (uint64_t)(((unsigned __int128)a + b) / 2);
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    int exhaustive = (argc > 1); /* any argument enables the 2^32 sweep */
    (void)argv;
    uint64_t mismatches = 0;
    uint64_t total_pairs = 0;

    /* Phase 1: exhaustive 16-bit pairs. */
    if (exhaustive) {
        uint64_t pairs = 0;
        double t0 = now_s();
        for (uint32_t a = 0; a <= 0xFFFFu; a++) {
            for (uint32_t b = 0; b <= 0xFFFFu; b++) {
                uint64_t got = avg_u64((uint64_t)a, (uint64_t)b);
                uint64_t want = oracle((uint64_t)a, (uint64_t)b);
                mismatches += (uint64_t)(got != want);
                pairs++;
            }
            if ((a & 0x1FFFu) == 0)
                printf("exhaustive: a=%5u of 65535, pairs=%" PRIu64
                       ", mismatches=%" PRIu64 "\n",
                       a, pairs, mismatches);
        }
        double dt = now_s() - t0;
        total_pairs += pairs;
        printf("phase 1 (exhaustive 16-bit pairs): %" PRIu64 " pairs, %"
               PRIu64 " mismatches, %.1f s (%.3f ns/pair)\n",
               pairs, mismatches, dt, dt * 1e9 / (double)pairs);
    }

    /* Phase 2: directed 64-bit edges. */
    {
        static const uint64_t edges[] = {
            0x0000000000000000ULL, 0x0000000000000001ULL,
            0x0000000000000002ULL, 0x0000000000000003ULL,
            0x00000000FFFFFFFFULL, 0x0000000100000000ULL,
            0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL,
            0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000ULL,
            0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL,
        };
        uint64_t n = sizeof(edges) / sizeof(edges[0]);
        uint64_t phase_mism = 0;
        uint64_t pairs = 0;
        for (uint64_t i = 0; i < n; i++) {
            for (uint64_t j = 0; j < n; j++) {
                uint64_t got = avg_u64(edges[i], edges[j]);
                uint64_t want = oracle(edges[i], edges[j]);
                if (got != want) {
                    phase_mism++;
                    printf("MISMATCH edge: a=%" PRIu64 " b=%" PRIu64
                           " got=%" PRIu64 " want=%" PRIu64 "\n",
                           edges[i], edges[j], got, want);
                }
                pairs++;
            }
        }
        mismatches += phase_mism;
        total_pairs += pairs;
        printf("phase 2 (directed 64-bit edges): %" PRIu64 " pairs, %"
               PRIu64 " mismatches\n",
               pairs, phase_mism);
    }

    /* Phase 3: 10M fixed-seed random 64-bit pairs. */
    {
        const uint64_t N = 10000000ULL;
        uint64_t phase_mism = 0;
        rng_state = 0x243F6A8885A308D3ULL; /* reset for reproducibility */
        for (uint64_t k = 0; k < N; k++) {
            uint64_t a = rng_next();
            uint64_t b = rng_next();
            uint64_t got = avg_u64(a, b);
            uint64_t want = oracle(a, b);
            if (got != want) {
                phase_mism++;
                if (phase_mism < 5)
                    printf("MISMATCH random: a=%" PRIu64 " b=%" PRIu64
                           " got=%" PRIu64 " want=%" PRIu64 "\n",
                           a, b, got, want);
            }
        }
        mismatches += phase_mism;
        total_pairs += N;
        printf("phase 3 (10M random 64-bit pairs): %" PRIu64 " pairs, %"
               PRIu64 " mismatches\n",
               N, phase_mism);
    }

    printf("correctness: %" PRIu64 " differential checks, %" PRIu64
           " mismatches\n",
           total_pairs, mismatches);

    if (mismatches != 0) {
        printf("TESTS FAILED\n");
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
