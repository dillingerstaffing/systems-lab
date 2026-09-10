#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "iso.h"

/* splitmix64 with a fixed seed; the seed is stated in PROOF.md and
 * baked in here so every build runs the identical input sequence. */
static uint64_t rng_state = 0x243F6A8885A308D3ULL;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Naive per-bit reference: find the lowest set bit by scanning. */
static uint64_t ref_iso_lowest(uint64_t x)
{
    for (unsigned b = 0; b < 64; b++) {
        if (x & ((uint64_t)1 << b)) {
            return (uint64_t)1 << b;
        }
    }
    return 0;
}

/* Independent naive popcount: count bits one at a time. No builtins. */
static unsigned ref_popcount(uint64_t x)
{
    unsigned n = 0;
    while (x) {
        n += (unsigned)(x & 1u);
        x >>= 1;
    }
    return n;
}

/* FNV-1a 64 over the result stream; the same value must come out of
 * the -O0, -O2, and sanitizer builds. */
static uint64_t fnv1a(uint64_t h, uint64_t v)
{
    h ^= v;
    h *= 0x100000001B3ULL;
    return h;
}

static long long total_cases = 0;
static long long nonzero_cases = 0;
static long long mismatches = 0;
static long long invariant_fails = 0;
static uint64_t checksum = 0xCBF29CE484222325ULL;

static void check(uint64_t x)
{
    uint64_t got = iso_lowest(x);
    uint64_t want = ref_iso_lowest(x);
    total_cases++;
    checksum = fnv1a(checksum, got);
    if (got != want) {
        mismatches++;
        if (mismatches < 5) {
            printf("MISMATCH x=%" PRIu64 " got=%" PRIu64 " want=%" PRIu64 "\n",
                   x, got, want);
        }
        return;
    }
    if (x != 0) {
        nonzero_cases++;
        /* Invariant 1: removing the lowest set bit drops the
         * popcount by exactly one. */
        if (ref_popcount(x - got) != ref_popcount(x) - 1) {
            invariant_fails++;
            printf("INVARIANT-FAIL popcount x=%" PRIu64 "\n", x);
        }
        /* Invariant 2: the isolated bit is a power of two. */
        if ((got & (got - 1)) != 0) {
            invariant_fails++;
            printf("INVARIANT-FAIL pow2 x=%" PRIu64 "\n", x);
        }
        /* Invariant 3: iso really divides x with no remainder and
         * no smaller nonzero power of two divides x. */
        if ((x & got) != got) {
            invariant_fails++;
            printf("INVARIANT-FAIL divides x=%" PRIu64 "\n", x);
        }
    } else if (got != 0) {
        /* x = 0 contract: return 0. */
        invariant_fails++;
        printf("INVARIANT-FAIL zero x=0 got=%" PRIu64 "\n", got);
    }
}

int main(void)
{
    /* Exhaustive 16-bit sweep. */
    for (uint64_t x = 0; x < 65536; x++) {
        check(x);
    }
    /* 1,000,000 fixed-seed 64-bit values. */
    for (long long i = 0; i < 1000000; i++) {
        check(splitmix64());
    }

    printf("cases=%lld nonzero=%lld mismatches=%lld invariant_fails=%lld\n",
           total_cases, nonzero_cases, mismatches, invariant_fails);
    printf("fnv1a_checksum=%016" PRIx64 "\n", checksum);

    /* Timing: -O2 loop over a pre-generated array so the PRNG is not
     * in the measured path. The measured time includes the array
     * load, the iso_lowest call, and one accumulate op per value. */
    static uint64_t vals[1000000];
    rng_state = 0x243F6A8885A308D3ULL;
    for (long long i = 0; i < 1000000; i++) {
        vals[i] = splitmix64();
    }
    struct timespec t0, t1;
    volatile uint64_t sink = 0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (long long i = 0; i < 1000000; i++) {
        sink ^= iso_lowest(vals[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                (double)(t1.tv_nsec - t0.tv_nsec);
    printf("timed: 1000000 values, %.3f ns total, %.3f ns/value\n",
           ns, ns / 1000000.0);
    if (sink == 0xDEADBEEFDEADBEEFULL) {
        printf("unreachable\n");
    }

    if (mismatches != 0 || invariant_fails != 0) {
        return 1;
    }
    return 0;
}
