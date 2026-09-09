/*
 * Differential test for mul_const32.
 *
 * Oracle: the native product (uint32_t)((uint64_t)x * k), used ONLY
 * as the oracle. mul_const32 itself never uses the multiply operator
 * for its computation, only shifts and adds.
 *
 * Verification plan:
 *
 *   1. All 256 constants k in 0..255, each tested against 1,000,000
 *      full-range 32-bit x values from a fixed-seed xorshift64*
 *      PRNG (seed 0x9E3779B97F4A7C15, fully reproducible):
 *      256,000,000 differential checks total. The k sweep covers the
 *      edge constants (0 gives 0, 1 gives x, 255 exercises all 8
 *      shift/add steps) and the x stream covers wrapping behavior
 *      near 2^32.
 *   2. A 64-bit FNV-1a checksum accumulates every output and must be
 *      identical across the -O0, -O2, and ASan+UBSan builds.
 *   3. Throughput: 100,000,000 timed multiplies at whatever build
 *      flags are in use, with k varying per iteration so the
 *      compiler cannot constant-fold the constant, and results
 *      accumulated into a sink so the loop cannot be removed.
 */
#include "mul_const.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define CASES_PER_K 1000000u
#define BENCH_CASES 100000000u

/* Fixed seed, stated here so the random stream is reproducible. */
#define SEED 0x9E3779B97F4A7C15ULL

static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1DULL;
}

static uint64_t checksum;

static void accumulate(uint32_t v)
{
    checksum ^= (uint64_t)v;
    checksum *= 1099511628211ULL;
}

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

int main(void)
{
    unsigned long long mismatches = 0;
    unsigned long long total = 0;
    unsigned k, i;
    uint64_t t0, t1;
    unsigned long long sink = 0;

    /* Step 1: the full sweep, timed. */
    rng_state = SEED;
    t0 = now_ns();
    for (k = 0; k < 256; ++k) {
        for (i = 0; i < CASES_PER_K; ++i) {
            uint32_t x = (uint32_t)rng_next();
            uint32_t got = mul_const32(x, (uint8_t)k);
            uint32_t want = (uint32_t)((uint64_t)x * (uint64_t)k);

            if (got != want) {
                if (mismatches < 10)
                    printf("MISMATCH: x=%" PRIu32 " k=%u: got=%" PRIu32
                           " want=%" PRIu32 "\n",
                           x, k, got, want);
                ++mismatches;
            }
            accumulate(got);
            ++total;
        }
    }
    t1 = now_ns();
    printf("sweep: k=0..255 cases_per_k=%u total=%llu mismatches=%llu "
           "ns_total=%" PRIu64 " ns_per_check=%.2f\n",
           CASES_PER_K, total, mismatches, t1 - t0,
           (double)(t1 - t0) / (double)total);

    /* Step 2: throughput with a varying k, 100M timed multiplies. */
    rng_state = SEED;
    t0 = now_ns();
    for (i = 0; i < BENCH_CASES; ++i) {
        uint32_t x = (uint32_t)rng_next();
        uint8_t k2 = (uint8_t)rng_next();
        sink += mul_const32(x, k2);
    }
    t1 = now_ns();

    printf("checksum=%" PRIu64 "\n", checksum);
    printf("throughput: values=%u ns_total=%" PRIu64
           " ns_per_multiply=%.2f mops=%.1f sink=%llu\n",
           BENCH_CASES, t1 - t0,
           (double)(t1 - t0) / (double)BENCH_CASES,
           (double)BENCH_CASES / ((double)(t1 - t0) / 1e9) / 1e6,
           sink);

    if (mismatches != 0) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
