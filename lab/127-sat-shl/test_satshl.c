/*
 * Differential test for sat_shl64 (branchless saturating left shift).
 *
 * Oracle: a plain conditional implementation, structurally different
 * from the branchless construction under test. It decides the k >= 64
 * case first, then checks the bits shifted out (x >> (64 - k)) for
 * k in 1..63; only when nothing is lost does it perform x << k. The
 * k == 0 case returns early so (64 - k) is never 64 in the oracle.
 *
 * Verification plan:
 *
 *   1. Exhaustive sweep: all 64 shift amounts k = 0..63 against all
 *      65,536 16-bit x values, 4,194,304 differential cases.
 *   2. 1,000,000 random (x, k) pairs from a fixed-seed splitmix64
 *      PRNG (seed 0x123456789ABCDEF0, fully reproducible): 64-bit x,
 *      k in 0..127 (k >> 7 masked), covering the k >= 64 path and
 *      high-bit x values the sweep cannot reach.
 *   3. A 64-bit FNV-1a checksum accumulates every verified result and
 *      must be identical across the -O0, -O2, and ASan+UBSan builds.
 *   4. Throughput: best of 5 trials at -O2, 20,000,000 timed values
 *      per trial, results accumulated into a sink so the loop cannot
 *      be optimized away. The timed loop includes the PRNG step, so
 *      the figure is the cost of one generate-and-shift case, not
 *      one bare sat_shl64 call.
 *
 * The Makefile `disasm` target additionally objdumps the -O2 object
 * and asserts the compiled sat_shl64 contains zero conditional jumps.
 */
#include "satshl.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define SWEEP_K 64u
#define SWEEP_X 65536u
#define RAND_CASES 1000000u
#define BENCH_CASES 20000000u
#define BENCH_TRIALS 5

/* Fixed seed, stated here so the random stream is reproducible. */
#define SEED 0x123456789ABCDEF0ULL

static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Conditional oracle, structurally different from sat_shl64. */
static uint64_t sat_shl64_ref(uint64_t x, unsigned k)
{
    if (k >= 64u)
        return (x != 0u) ? UINT64_MAX : 0u;
    if (k == 0u)
        return x;
    if ((x >> (64u - k)) != 0u)
        return UINT64_MAX;
    return x << k;
}

/* 64-bit FNV-1a over the result stream. */
static uint64_t checksum;

static void checksum_add(uint64_t v)
{
    checksum ^= v;
    checksum *= 1099511628211ULL;
}

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void)
{
    uint64_t checks = 0;
    uint64_t mismatches = 0;
    uint64_t k, xv;
    unsigned trial;
    double best_ns = 0.0;
    uint64_t sink = 0;

    checksum = 14695981039346656037ULL;
    rng_state = SEED;

    /* 1. Exhaustive sweep: every k in 0..63, every 16-bit x. */
    for (k = 0; k < SWEEP_K; k++) {
        for (xv = 0; xv < SWEEP_X; xv++) {
            uint64_t got = sat_shl64((uint64_t)xv, (unsigned)k);
            uint64_t want = sat_shl64_ref((uint64_t)xv, (unsigned)k);
            if (got != want)
                mismatches++;
            checksum_add(got);
            checks++;
        }
    }

    /* 2. Random 64-bit x, k in 0..127, fixed seed. */
    for (xv = 0; xv < RAND_CASES; xv++) {
        uint64_t x = rng_next();
        unsigned kk = (unsigned)(rng_next() & 127u);
        uint64_t got = sat_shl64(x, kk);
        uint64_t want = sat_shl64_ref(x, kk);
        if (got != want)
            mismatches++;
        checksum_add(got);
        checks++;
    }

    printf("sweep_k=0..63 sweep_x=0..65535 random_pairs=%u total=%" PRIu64
           " mismatches=%" PRIu64 "\n",
           RAND_CASES, checks, mismatches);
    printf("checksum=%" PRIu64 "\n", checksum);

    /* 3. Throughput: best of 5 trials. */
    for (trial = 0; trial < BENCH_TRIALS; trial++) {
        uint64_t i;
        double start, elapsed, ns_per;
        rng_state = SEED ^ (0x9E3779B97F4A7C15ULL * (uint64_t)(trial + 1));
        start = now_ns();
        for (i = 0; i < BENCH_CASES; i++) {
            uint64_t x = rng_next();
            unsigned kk = (unsigned)(rng_next() & 127u);
            sink += sat_shl64(x, kk);
        }
        elapsed = now_ns() - start;
        ns_per = elapsed / (double)BENCH_CASES;
        if (trial == 0 || ns_per < best_ns)
            best_ns = ns_per;
        printf("trial %u: ns_per_value=%.3f\n", trial, ns_per);
    }
    printf("throughput: best_of_%d trials values=%u ns_per_value=%.3f sink=%"
           PRIu64 "\n",
           BENCH_TRIALS, BENCH_CASES, best_ns, sink);
    printf("%s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
