#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mask.h"

#define WIDTHS 65 /* n = 0..64 */
#define DRAWS_PER_WIDTH 1000000
#define THROUGHPUT_ITERS 100000000

/* Independent reference: sets each of the n high bits one at a time. */
static uint64_t naive_mask(int n)
{
    uint64_t r = 0;
    for (int i = 0; i < n; i++)
        r |= 1ULL << (63 - i);
    return r;
}

/* Independent popcount: strips set bits one at a time, no builtins. */
static int popcount64(uint64_t x)
{
    int c = 0;
    while (x) {
        x &= x - 1;
        c++;
    }
    return c;
}

/* The low m bits set, for 0 <= m <= 64; used to clear-proof random draws. */
static uint64_t low_mask(int m)
{
    return (m == 64) ? ~0ULL : ((1ULL << m) - 1);
}

/* Fixed-seed splitmix64 so every run draws the same stream. */
static uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static double now_sec(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

int main(void)
{
    uint64_t state = 0x123456789ABCDEF0ULL;
    uint64_t fnv = 0xCBF29CE484222325ULL; /* FNV-1a 64-bit offset basis */
    uint64_t checks = 0, mismatches = 0, invariant_failures = 0;
    uint64_t random_failures = 0;

    double t0 = now_sec();
    for (int n = 0; n < WIDTHS; n++) {
        uint64_t low = low_mask(64 - n); /* must stay zero after masking */
        for (int k = 0; k < DRAWS_PER_WIDTH; k++) {
            uint64_t v = splitmix64(&state);
            uint64_t got = mask_above64(n);
            uint64_t ref = naive_mask(n);
            if (got != ref) {
                mismatches++;
                if (mismatches <= 5)
                    printf("mismatch: n=%d got=%016" PRIx64
                           " ref=%016" PRIx64 "\n",
                           n, got, ref);
            }
            /* The mask must hold exactly n set bits. */
            if (popcount64(got) != n)
                invariant_failures++;
            /* A high-bit field is contiguous: OR-ing the mask with one
               less fills every low bit and no others. */
            if (!(got == 0ULL || (got | (got - 1)) == ~0ULL))
                invariant_failures++;
            /* Masking a random word must leave no low bits standing. */
            if (((v & got) & low) != 0ULL)
                random_failures++;
            fnv ^= got;
            fnv *= 0x100000001B3ULL;
            checks++;
        }
    }
    double t1 = now_sec();

    /* Throughput loop: PRNG draw plus mask per value, so this measures
       an upper bound on the cost of the mask alone. */
    uint64_t acc = 0;
    double u0 = now_sec();
    for (uint64_t i = 0; i < THROUGHPUT_ITERS; i++)
        acc ^= splitmix64(&state) & mask_above64(i % WIDTHS);
    double u1 = now_sec();

    printf("cases=%llu\n", (unsigned long long)checks);
    printf("mismatches=%llu\n", (unsigned long long)mismatches);
    printf("invariant_failures=%llu\n", (unsigned long long)invariant_failures);
    printf("random_failures=%llu\n", (unsigned long long)random_failures);
    printf("fnv1a=%016llx\n", (unsigned long long)fnv);
    printf("verification_time=%.3f s\n", t1 - t0);
    printf("throughput_ns_per_value=%.3f\n",
           (u1 - u0) * 1e9 / (double)THROUGHPUT_ITERS);
    printf("throughput_acc=%016llx\n", (unsigned long long)acc);
    return (mismatches == 0 && invariant_failures == 0 &&
            random_failures == 0) ? 0 : 1;
}
