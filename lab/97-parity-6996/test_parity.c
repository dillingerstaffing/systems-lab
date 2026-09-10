#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "parity.h"

/* Naive per-bit loop reference: ground truth for parity. */
static int ref_parity32(uint32_t x)
{
    int p = 0;
    for (int i = 0; i < 32; i++)
        p ^= (int)((x >> i) & 1u);
    return p;
}

/* splitmix64: deterministic stream of test inputs. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;
static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 1469598103934665603ULL;
static void fnv(uint64_t v)
{
    fnv1a ^= v;
    fnv1a *= 1099511628211ULL;
}

int main(void)
{
    long long checks = 0;
    long long mismatches = 0;

    /* 1. Exhaustive 16-bit inputs. */
    for (uint32_t x = 0; x < 65536u; x++) {
        if (parity32(x) != ref_parity32(x)) {
            printf("MISMATCH x=%08x\n", x);
            mismatches++;
        }
        fnv((uint64_t)parity32(x));
        checks++;
    }

    /* 2. One million fixed-seed random 32-bit values. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 1000000; i++) {
        uint32_t x = (uint32_t)splitmix64();
        if (parity32(x) != ref_parity32(x)) {
            printf("MISMATCH x=%08x\n", x);
            mismatches++;
        }
        fnv((uint64_t)parity32(x));
        checks++;
    }

    /* 3. Homomorphism parity(a^b) == parity(a)^parity(b) on 1M pairs,
       distinct fixed seed so the pairs are independent of section 2. */
    rng_state = 0x0FEDCBA987654321ULL;
    long long hom_checks = 0;
    for (int i = 0; i < 1000000; i++) {
        uint32_t a = (uint32_t)splitmix64();
        uint32_t b = (uint32_t)splitmix64();
        if (parity32(a ^ b) != (parity32(a) ^ parity32(b))) {
            printf("HOM-MISMATCH a=%08x b=%08x\n", a, b);
            mismatches++;
        }
        fnv((uint64_t)parity32(a ^ b));
        hom_checks++;
    }

    printf("checks=%lld hom_checks=%lld mismatches=%lld fnv1a=%016llx\n",
           checks, hom_checks, mismatches, (unsigned long long)fnv1a);

#ifdef BENCH
    /* Throughput: 25M values at -O2, best-of-5. */
    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        rng_state = 0x9E3779B97F4A7C15ULL + (uint64_t)rep;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile int sink = 0;
        for (int i = 0; i < 25000000; i++)
            sink ^= parity32((uint32_t)splitmix64());
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    printf("bench: %.2f ns/value (%.1f Mvalues/s over 25M timed values, best of 5)\n",
           best / 25000000.0, 25000000.0 / (best / 1e3));
#endif

    return mismatches == 0 ? 0 : 1;
}
