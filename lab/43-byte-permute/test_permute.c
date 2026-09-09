#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "permute.h"

/* Fixed permutation P = (2,5,0,7,1,6,3,4) and its inverse INV = (2,4,0,6,7,1,5,3).
 * The oracle is an independent table-driven byte loop; the functions under
 * test use no tables. */
static const uint8_t P[8]   = { 2, 5, 0, 7, 1, 6, 3, 4 };
static const uint8_t INV[8] = { 2, 4, 0, 6, 7, 1, 5, 3 };

/* Independent oracle: route bytes through plain uint8_t arrays. */
static uint64_t oracle_permute(uint64_t x, const uint8_t tab[8])
{
    uint8_t in[8];
    uint8_t out[8];
    uint64_t r = 0;
    int i;

    for (i = 0; i < 8; i++)
        in[i] = (uint8_t)(x >> (8 * i));
    for (i = 0; i < 8; i++)
        out[i] = in[tab[i]];
    for (i = 0; i < 8; i++)
        r |= (uint64_t)out[i] << (8 * i);
    return r;
}

/* Fixed-seed PRNG so every run checks the same values. */
static uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv1a_update(uint64_t hash, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++) {
        hash ^= (uint8_t)(v >> (8 * i));
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#define N_RANDOM 1000000UL
#define N_EXHAUST 65536UL

int main(void)
{
    uint64_t cases = 0, mismatches = 0, invariant_failures = 0;
    uint64_t hash = 0xCBF29CE484222325ULL; /* FNV-1a offset basis */
    uint64_t rng_state = 0x123456789ABCDEF0ULL;
    uint64_t i, x;
    uint64_t sink = 0;
    double t0, t1;
    int pass;

    /* Verification pass: differential checks plus the round-trip invariant
     * in both directions on every input. */
    for (pass = 0; pass < 2; pass++) {
        uint64_t n = (pass == 0) ? N_EXHAUST : N_RANDOM;
        for (i = 0; i < n; i++) {
            uint64_t p, q;
            if (pass == 0)
                x = i;                 /* exhaustive 16-bit inputs */
            else
                x = splitmix64(&rng_state); /* 1M fixed-seed 64-bit values */

            p = permute64(x);
            if (p != oracle_permute(x, P))
                mismatches++;
            q = inv_permute64(x);
            if (q != oracle_permute(x, INV))
                mismatches++;
            if (inv_permute64(p) != x || permute64(q) != x)
                invariant_failures++;

            hash = fnv1a_update(hash, p);
            hash = fnv1a_update(hash, q);
            cases++;
        }
    }

    /* Timed pass at the same -O level: permute and inverse only, so the
     * measured time is the cost of the two functions, not the oracle. */
    rng_state = 0x123456789ABCDEF0ULL;
    t0 = now_s();
    for (i = 0; i < N_RANDOM; i++) {
        x = splitmix64(&rng_state);
        sink ^= permute64(x) ^ inv_permute64(x);
    }
    t1 = now_s();

    printf("cases=%llu\n", (unsigned long long)cases);
    printf("mismatches=%llu\n", (unsigned long long)mismatches);
    printf("invariant_failures=%llu\n", (unsigned long long)invariant_failures);
    printf("fnv1a=%016llx\n", (unsigned long long)hash);
    printf("bench_time=%.3f s\n", (t1 - t0) >= 0.0 ? (t1 - t0) : 0.0);
    printf("throughput_ns_per_value=%.3f\n",
           (t1 - t0) * 1e9 / (double)(2 * N_RANDOM));
    printf("throughput_acc=%016llx\n", (unsigned long long)sink);
    printf("exit_code=%d\n", (mismatches == 0 && invariant_failures == 0) ? 0 : 1);
    return (mismatches == 0 && invariant_failures == 0) ? 0 : 1;
}
