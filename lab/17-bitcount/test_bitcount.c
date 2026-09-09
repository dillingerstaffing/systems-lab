#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "bitcount.h"

/* Fixed-seed splitmix64: reproducible stream of 64-bit values. */
static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Differential check of popcount64 against the independent oracle
 * __builtin_popcountll. Returns 1 on mismatch, 0 on match. */
static int check(uint64_t x)
{
    uint32_t got = popcount64(x);
    uint32_t want = (uint32_t)__builtin_popcountll((unsigned long long)x);
    return got != want;
}

#define N_RANDOM 10000000UL

int main(void)
{
    unsigned long mismatches = 0;
    unsigned long cases = 0;
    uint64_t sink = 0; /* checksum so the compiler cannot drop the loop */

    /* 1. Directed edge cases: the identity boundaries. */
    static const uint64_t directed0[] = {
        0x0000000000000000ULL, /* zero */
        0xFFFFFFFFFFFFFFFFULL, /* all ones */
        0x5555555555555555ULL, /* alternating, every field boundary exercised */
        0xAAAAAAAAAAAAAAAAULL, /* alternating, shifted by one */
    };
    for (size_t i = 0; i < sizeof(directed0) / sizeof(directed0[0]); i++) {
        mismatches += (unsigned long)check(directed0[i]);
        cases++;
    }

    for (int b = 0; b < 64; b++) {
        mismatches += (unsigned long)check(1ULL << b); /* every single-bit position */
        cases++;
    }

    for (int k = 0; k <= 64; k++) {
        /* 2^k - 1: fields fill one bit at a time, every nibble boundary crossed */
        uint64_t v = (k == 64) ? 0xFFFFFFFFFFFFFFFFULL : ((k == 0) ? 0ULL : ((1ULL << k) - 1ULL));
        mismatches += (unsigned long)check(v);
        cases++;
    }

    /* 2. Random differential: 10,000,000 fixed-seed 64-bit values. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (unsigned long i = 0; i < N_RANDOM; i++) {
        uint64_t x = rng_next();
        mismatches += (unsigned long)check(x);
        sink += popcount64(x);
    }
    cases += N_RANDOM;

    printf("total_cases=%lu mismatches=%lu checksum=%llu\n",
           cases, mismatches, (unsigned long long)sink);

    /* 3. Throughput: timed loop over fresh fixed-seed values. */
    rng_state = 0xFEDCBA9876543210ULL;
    struct timespec t0, t1;
    const unsigned long N_TIMED = 100000000UL;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (unsigned long i = 0; i < N_TIMED; i++) {
        sink += popcount64(rng_next());
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                (double)(t1.tv_nsec - t0.tv_nsec);
    printf("timed_values=%lu ns_total=%.0f ns_per_value=%.2f Mvalues_per_sec=%.1f\n",
           N_TIMED, ns, ns / (double)N_TIMED,
           (double)N_TIMED / ns * 1e3);
    printf("checksum=%llu\n", (unsigned long long)sink);

    if (mismatches != 0)
        return 1;
    printf("PASS\n");
    return 0;
}
