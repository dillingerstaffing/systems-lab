#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mask.h"

#define WIDTHS 65 /* n = 0..64 */
#define DRAWS_PER_WIDTH 1000000
#define THROUGHPUT_ITERS 50000000

/* Independent reference: sets each of the low n bits one at a time. */
static uint64_t naive_mask(int n)
{
    uint64_t r = 0;
    for (int i = 0; i < n; i++)
        r |= 1ULL << i;
    return r;
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

    double t0 = now_sec();
    for (int n = 0; n < WIDTHS; n++) {
        for (int k = 0; k < DRAWS_PER_WIDTH; k++) {
            /*
             * One splitmix64 draw per case. The mask under test is a
             * function of n only; the draw varies each case through
             * the full loop body without changing the result, which
             * is why the checksum below is exactly reproducible.
             */
            (void)splitmix64(&state);
            uint64_t got = mask_below(n);
            uint64_t ref = naive_mask(n);
            if (got != ref) {
                mismatches++;
                if (mismatches <= 5)
                    printf("mismatch: n=%d got=%016" PRIx64
                           " ref=%016" PRIx64 "\n",
                           n, got, ref);
            }
            /* 2^n - 1 and 2^n share no bits, for every n, including n = 64. */
            if (((got + 1) & got) != 0ULL)
                invariant_failures++;
            fnv ^= got;
            fnv *= 0x100000001B3ULL;
            checks++;
        }
    }
    double t1 = now_sec();

    /* Dedicated throughput loop for mask_below itself. */
    uint64_t acc = 0;
    double u0 = now_sec();
    for (int i = 0; i < THROUGHPUT_ITERS; i++)
        acc ^= mask_below(i % WIDTHS);
    double u1 = now_sec();

    printf("cases=%llu\n", (unsigned long long)checks);
    printf("mismatches=%llu\n", (unsigned long long)mismatches);
    printf("invariant_failures=%llu\n", (unsigned long long)invariant_failures);
    printf("fnv1a=%016llx\n", (unsigned long long)fnv);
    printf("verification_time=%.3f s\n", t1 - t0);
    printf("throughput_ns_per_value=%.3f\n",
           (u1 - u0) * 1e9 / THROUGHPUT_ITERS);
    printf("throughput_acc=%016llx\n", (unsigned long long)acc);
    return (mismatches == 0 && invariant_failures == 0) ? 0 : 1;
}
