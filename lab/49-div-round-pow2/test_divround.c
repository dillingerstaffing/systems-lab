#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "divround.h"

/* Independent reference: round-half-up(x / 2^k) = floor(x/2^k + 1/2),
 * and floor(x/2^k + 1/2) = (2x + 2^k) / 2^(k+1) in integer arithmetic.
 * Uses only multiply/add/divide, never the add-half-then-shift form of
 * the implementation, and runs entirely in 64 bits. */
static uint32_t ref_round_half_up_div_pow2(uint32_t x, unsigned k)
{
    uint64_t num = ((uint64_t)x << 1) + ((uint64_t)1 << k);
    return (uint32_t)(num / ((uint64_t)1 << (k + 1)));
}

/* splitmix64: deterministic RNG for the random differential test. */
static uint64_t rng_state = 0x243F6A8885A308D3ULL; /* fixed seed */
static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a 64 over every result, so all builds must agree bit for bit. */
static uint64_t fnv = 0xCBF29CE484222325ULL;
static void fnv_feed(uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        fnv ^= (uint64_t)((v >> (8 * i)) & 0xFF);
        fnv *= 0x100000001B3ULL;
    }
}

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int check_vector(uint32_t x, unsigned k, uint32_t expect,
                        const char *name)
{
    uint32_t got = round_half_up_div_pow2(x, k);
    if (got != expect) {
        printf("vector %s: x=%u k=%u -> %u, expected %u: FAIL\n", name, x, k,
               got, expect);
        return 1;
    }
    printf("vector %s: x=%u k=%u -> %u: pass\n", name, x, k, got);
    return 0;
}

int main(void)
{
    int fails = 0;
    uint64_t checks = 0;
    uint64_t mismatches = 0;

    /* Hand-checked vectors, including ties and both edge cases. */
    fails += check_vector(3, 1, 2, "1.5 rounds up");
    fails += check_vector(2, 1, 1, "exact 1");
    fails += check_vector(6, 2, 2, "1.5 rounds up at k=2");
    fails += check_vector(5, 2, 1, "1.25 rounds down");
    fails += check_vector(7, 2, 2, "1.75 rounds up");
    fails += check_vector(0, 7, 0, "zero stays zero");
    fails += check_vector(64, 7, 1, "tie x=2^(k-1) rounds up");
    fails += check_vector(63, 7, 0, "just below tie rounds down");
    fails += check_vector(1, 1, 1, "0.5 tie at k=1 rounds up");
    fails += check_vector(0xDEADBEEF, 0, 0xDEADBEEF, "k=0 identity");
    fails += check_vector(0, 0, 0, "k=0 zero");
    fails += check_vector(0x80000000, 32, 1, "k=32 tie x=2^31 rounds up");
    fails += check_vector(0x7FFFFFFF, 32, 0, "k=32 just below tie");
    fails += check_vector(0xFFFFFFFF, 32, 1, "k=32 top of range");
    fails += check_vector(0, 32, 0, "k=32 zero");
    fails += check_vector(0xFFFFFFFF, 31, 2,
                          "k=31 full-range, needs 64-bit add");
    fails += check_vector(0xFFFFFFFF, 1, 0x80000000,
                          "k=1 max x (2^32-1)/2 rounds up");
    printf("vectors: %s\n", fails ? "FAIL" : "17/17 passed");

    /* Exhaustive: all 16-bit x, k = 1..15. */
    for (uint32_t x = 0; x <= 0xFFFF; x++) {
        for (unsigned k = 1; k <= 15; k++) {
            uint32_t got = round_half_up_div_pow2(x, k);
            uint32_t ref = ref_round_half_up_div_pow2(x, k);
            if (got != ref) {
                if (mismatches < 5)
                    printf("MISMATCH x=%u k=%u got=%u ref=%u\n", x, k, got,
                           ref);
                mismatches++;
            }
            fnv_feed(got);
            checks++;
        }
    }
    printf("exhaustive: %llu checks, %llu mismatches\n",
           (unsigned long long)checks, (unsigned long long)mismatches);

    /* Random: 1,000,000 full 32-bit x, k in 1..31, fixed seed
     * 0x243F6A8885A308D3 (splitmix64 above). */
    for (uint64_t i = 0; i < 1000000; i++) {
        uint32_t x = (uint32_t)rng_next();
        unsigned k = 1 + (unsigned)(rng_next() % 31);
        uint32_t got = round_half_up_div_pow2(x, k);
        uint32_t ref = ref_round_half_up_div_pow2(x, k);
        if (got != ref) {
            if (mismatches < 10)
                printf("MISMATCH x=%u k=%u got=%u ref=%u\n", x, k, got, ref);
            mismatches++;
        }
        fnv_feed(got);
        checks++;
    }
    printf("random: %llu checks, %llu mismatches (seed 0x243F6A8885A308D3)\n",
           (unsigned long long)checks, (unsigned long long)mismatches);
    printf("total: %llu checks, %llu mismatches\n",
           (unsigned long long)checks, (unsigned long long)mismatches);
    printf("fnv1a-64 over all results: 0x%llx\n", (unsigned long long)fnv);

    /* Throughput: 40M calls with k varying 1..31 so the compiler cannot
     * fold the loop to a constant. This is a ceiling on the true cost,
     * not a floor: the loop body includes the modulo and the call. */
    const uint64_t N = 40000000ULL;
    volatile uint32_t sink = 0;
    uint64_t t0 = now_ns();
    for (uint64_t i = 0; i < N; i++)
        sink += round_half_up_div_pow2((uint32_t)i, 1 + (unsigned)(i % 31));
    uint64_t t1 = now_ns();
    double secs = (double)(t1 - t0) / 1e9;
    printf("benchmark: %llu calls in %.3f s = %.1f ns/value (sink=%u)\n",
           (unsigned long long)N, secs, secs * 1e9 / (double)N, sink);

    if (fails || mismatches) {
        printf("TESTS FAILED\n");
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
