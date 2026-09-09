#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "parity.h"

/* Naive per-bit reference: the ground truth every check compares to. */
static int naive_parity64(uint64_t x)
{
    int p = 0;
    for (int i = 0; i < 64; i++)
        p ^= (int)((x >> i) & 1u);
    return p;
}

/*
 * splitmix64 PRNG. The run is reproducible because the state is seeded
 * with a fixed constant before each phase.
 */
static uint64_t rng_state;
static void rng_seed(uint64_t seed) { rng_state = seed; }
static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a 64-bit checksum over the result stream. */
static uint64_t fnv = 0xCBF29CE484222325ULL;
static void fnv_feed_bit(int bit)
{
    fnv ^= (uint64_t)(unsigned)bit;
    fnv *= 0x100000001B3ULL;
}

#define N_RAND 1000000u
#define N_PAIRS 1000000u
#define N_REPS 25

static uint64_t vals[N_RAND];

int main(void)
{
    unsigned mismatches = 0;
    unsigned long long total_checks = 0;

    /* Phase 1: exhaustive 16-bit inputs, differential vs naive. */
    for (uint32_t v = 0; v < 65536u; v++) {
        int p = parity64((uint64_t)v);
        int r = naive_parity64((uint64_t)v);
        if (p != r) {
            mismatches++;
            if (mismatches < 8)
                printf("MISMATCH 16-bit v=%u impl=%d ref=%d\n", v, p, r);
        }
        fnv_feed_bit(p);
        total_checks++;
    }

    /* Phase 2: 1M fixed-seed random 64-bit values, differential vs naive. */
    rng_seed(0x123456789ABCDEF0ULL);
    for (uint32_t i = 0; i < N_RAND; i++) {
        uint64_t x = rng_next();
        vals[i] = x;
        int p = parity64(x);
        int r = naive_parity64(x);
        if (p != r) {
            mismatches++;
            if (mismatches < 8)
                printf("MISMATCH rand i=%u impl=%d ref=%d\n", i, p, r);
        }
        fnv_feed_bit(p);
        total_checks++;
    }

    /*
     * Phase 3: homomorphism parity(a ^ b) == parity(a) ^ parity(b) on
     * 1M pairs from a distinct fixed seed. (XOR of bit-strings adds bit
     * counts mod 2, so the identity must hold for every pair.)
     */
    rng_seed(0xDEADBEEFCAFEBABEULL);
    for (uint32_t i = 0; i < N_PAIRS; i++) {
        uint64_t a = rng_next();
        uint64_t b = rng_next();
        int left = parity64(a ^ b);
        int right = parity64(a) ^ parity64(b);
        if (left != right) {
            mismatches++;
            if (mismatches < 8)
                printf("MISMATCH homomorphism i=%u\n", i);
        }
        fnv_feed_bit(left);
        total_checks++;
    }

    /* Timing of the fold alone over the stored 1M values. */
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    unsigned long long sink = 0;
    for (unsigned rep = 0; rep < N_REPS; rep++)
        for (uint32_t i = 0; i < N_RAND; i++)
            sink += (unsigned)parity64(vals[i]);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                (double)(t1.tv_nsec - t0.tv_nsec);
    double per = ns / ((double)N_RAND * (double)N_REPS);

    printf("total_checks=%llu mismatches=%u fnv1a=%016llx\n",
           total_checks, mismatches, (unsigned long long)fnv);
    printf("timed_values=%llu ns_total=%.0f ns_per_value=%.2f Mvalues_per_sec=%.1f\n",
           (unsigned long long)N_RAND * (unsigned long long)N_REPS,
           ns, per, 1000.0 / per);
    printf("sink=%llu\n", sink);
    puts(mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
