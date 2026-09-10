/*
 * Differential test for longest_zero_run.
 *
 * Oracle: a naive bit-by-bit loop that walks all 64 bits from low to
 * high, tracking the current and best zero-run lengths. It uses no bit
 * tricks, so it is an independent reimplementation of the contract.
 *
 * Verification plan:
 *
 *   1. Directed cases with hand-computed answers: x = 0 -> 64,
 *      x = 0xFFFFFFFFFFFFFFFF -> 0, x = 0xAAAAAAAAAAAAAAAA -> 1,
 *      x = 0x8000000000000001 -> 62, x = 0x00000000FFFFFFFF -> 32.
 *      All 64 single-set-bit words (1ULL << k, expected longest
 *      zero-run max(k, 63-k)) and all 64 single-clear-bit words
 *      (~(1ULL << k), expected 1). These cover every bit position.
 *   2. Exhaustive sweep over 0..65535 (all 2^16 values, covering the
 *      all-zeros input) against the naive oracle.
 *   3. 1,000,000 full-range 64-bit values from a fixed-seed splitmix64
 *      PRNG (seed 0x243F6A8885A308D3, fully reproducible) against the
 *      naive oracle.
 *   4. Throughput: 100,000,000 timed values at whatever build flags
 *      are in use, with the results accumulated into a sink so the
 *      loop cannot be optimized away.
 *
 * A 64-bit FNV-1a checksum accumulates every verified output and must
 * be identical across the -O0, -O2, and ASan+UBSan builds, proving the
 * tested behavior does not depend on optimization level or
 * instrumentation.
 */
#include "zero_run.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define RAND_CASES 1000000u
#define BENCH_CASES 100000000u

/* Fixed splitmix64 seed, stated so the random stream is reproducible. */
#define SEED 0x243F6A8885A308D3ULL

static uint64_t rng_state = SEED;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/*
 * Naive reference: walk all 64 bits, tracking the current and best
 * zero-run lengths. A fresh loop, no shift/AND cascade.
 */
static unsigned ref_longest_zero_run(uint64_t x)
{
    unsigned best = 0u, cur = 0u;
    int i;

    for (i = 0; i < 64; ++i) {
        if ((x & 1ULL) == 0ULL) {
            ++cur;
            if (cur > best)
                best = cur;
        } else {
            cur = 0u;
        }
        x >>= 1;
    }
    return best;
}

/* FNV-1a, offset basis; accumulates every verified output. */
static uint64_t checksum = 1469598103934665603ULL;

static void accumulate(unsigned v)
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
    unsigned long mismatches = 0;
    unsigned long checks = 0;
    uint64_t v;
    int k;
    uint64_t t0, t1;
    unsigned long long sink = 0;

    /* Step 1: directed cases with hand-computed answers. */
    {
        unsigned z = longest_zero_run(0ULL);
        unsigned o = longest_zero_run(0xFFFFFFFFFFFFFFFFULL);
        unsigned alt = longest_zero_run(0xAAAAAAAAAAAAAAAAULL);
        unsigned hi = longest_zero_run(0x8000000000000001ULL);
        unsigned half = longest_zero_run(0x00000000FFFFFFFFULL);

        if (z != 64u || o != 0u || alt != 1u || hi != 62u ||
            half != 32u) {
            printf("FAIL: directed zeros=%u ones=%u alt=%u hi=%u half=%u\n",
                   z, o, alt, hi, half);
            return 1;
        }
        printf("directed: zeros=64 ones=0 alt=1 hi=62 half=32\n");
        accumulate(z);
        accumulate(o);
        accumulate(alt);
        accumulate(hi);
        accumulate(half);
        checks += 5;

        /*
         * Single set bit at position k: the two zero stretches are k
         * bits below and 63-k bits above, so the longest zero run is
         * max(k, 63-k).
         */
        for (k = 0; k < 64; ++k) {
            unsigned want = (unsigned)(k > 63 - k ? k : 63 - k);
            unsigned got = longest_zero_run(1ULL << (unsigned)k);

            if (got != want) {
                printf("FAIL: single bit k=%d: got=%u want=%u\n",
                       k, got, want);
                return 1;
            }
            accumulate(got);
            ++checks;
        }

        /*
         * Single clear bit at position k: the word has exactly one
         * zero, so the longest zero run is 1.
         */
        for (k = 0; k < 64; ++k) {
            unsigned got = longest_zero_run(~(1ULL << (unsigned)k));

            if (got != 1u) {
                printf("FAIL: single clear k=%d: got=%u\n", k, got);
                return 1;
            }
            accumulate(got);
            ++checks;
        }
        printf("singlebit: 64 positions OK, singleclear: 64 positions OK\n");
    }

    /* Step 2: exhaustive 16-bit sweep against the naive oracle. */
    for (v = 0; v <= 0xFFFFULL; ++v) {
        unsigned got = longest_zero_run(v);
        unsigned want = ref_longest_zero_run(v);

        if (got != want) {
            printf("MISMATCH: zero_run(%" PRIu64 "): got=%u want=%u\n",
                   v, got, want);
            ++mismatches;
        }
        accumulate(got);
        ++checks;
    }

    /* Step 3: 1,000,000 fixed-seed splitmix64 full-range values. */
    rng_state = SEED;
    for (v = 0; v < RAND_CASES; ++v) {
        uint64_t x = rng_next();
        unsigned got = longest_zero_run(x);
        unsigned want = ref_longest_zero_run(x);

        if (got != want) {
            printf("MISMATCH: zero_run(0x%016" PRIx64 "): got=%u want=%u\n",
                   x, got, want);
            ++mismatches;
        }
        accumulate(got);
        ++checks;
    }

    printf("exhaustive_16bit=65536 random_64bit=%u total=%lu mismatches=%lu\n",
           RAND_CASES, checks, mismatches);
    printf("checksum=%" PRIu64 "\n", checksum);

    if (mismatches != 0) {
        printf("FAIL\n");
        return 1;
    }

    /* Step 4: throughput. */
    rng_state = SEED;
    t0 = now_ns();
    for (v = 0; v < BENCH_CASES; ++v)
        sink += longest_zero_run(rng_next());
    t1 = now_ns();
    printf("throughput: values=%u ns_total=%" PRIu64
           " ns_per_value=%.2f sink=%llu\n",
           BENCH_CASES, t1 - t0,
           (double)(t1 - t0) / (double)BENCH_CASES, sink);
    printf("PASS\n");
    return 0;
}
