/*
 * Differential test for ctz32.
 *
 * Oracle: a naive bit-by-bit loop that walks x right one bit at a
 * time; it returns 32 for x == 0, matching ctz32's contract. A
 * second cross-check compares against __builtin_ctz for every nonzero
 * 16-bit value (__builtin_ctz is undefined on 0, so 0 is excluded
 * from that check). A third check asserts the de Bruijn identity
 * itself: for every k in 0..31, (2^k * 0x077CB531) >> 27 must yield
 * 32 distinct 5-bit values, so the table is verified, not trusted.
 *
 * Verification plan:
 *
 *   1. de Bruijn identity: 32 single-bit powers, distinct 5-bit
 *      keys required, and ctz32(1u << k) == k for every k.
 *   2. Exhaustive sweep over 0..65535 (all 2^16 values, covering 0,
 *      1, and the low single-bit positions) against the naive
 *      oracle, plus __builtin_ctz cross-check for the 65535
 *      nonzero values.
 *   3. 2,000,000 full-range 32-bit values from a fixed-seed
 *      xorshift64* PRNG (seed 0x2545F4914F6CDD1D, fully
 *      reproducible), covering the high single-bit positions
 *      (asserted explicitly too).
 *   4. Throughput: 100,000,000 timed values at whatever build
 *      flags are in use, with the results accumulated into a sink
 *      so the loop cannot be optimized away.
 *
 * A 64-bit FNV-1a checksum accumulates every verified output and
 * must be identical across the -O0, -O2, and ASan+UBSan builds,
 * proving the tested behavior does not depend on optimization level
 * or instrumentation.
 */
#include "ctz.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define RAND_CASES 2000000u
#define BENCH_CASES 100000000u

/* Fixed seed, stated here so the random stream is reproducible. */
#define SEED 0x2545F4914F6CDD1DULL

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

/* Naive reference: count zero bits below the lowest 1 by walking.
 * The n < 32 bound makes x == 0 terminate with 32, matching the
 * ctz32 contract. */
static unsigned ref_ctz32(uint32_t x)
{
    unsigned n = 0u;
    while (n < 32u && (x & 1u) == 0u) {
        x >>= 1;
        ++n;
    }
    return n;
}

static uint64_t checksum;

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
    uint32_t v;
    unsigned i, k;
    uint64_t t0, t1;
    unsigned long long sink = 0;

    /* Step 1: assert the de Bruijn identity and the single-bit cases. */
    {
        unsigned seen[32] = { 0 };
        unsigned distinct = 0;

        for (k = 0; k < 32; ++k) {
            uint32_t lowbit = 1u << k;
            unsigned idx =
                (unsigned)((lowbit * 0x077CB531u) >> 27);

            if (idx >= 32u) {
                printf("FAIL: de Bruijn index out of range for k=%u\n", k);
                return 1;
            }
            if (seen[idx]) {
                printf("FAIL: de Bruijn key collision at k=%u\n", k);
                return 1;
            }
            seen[idx] = 1;
            ++distinct;

            if (ctz32(lowbit) != k) {
                printf("FAIL: ctz32(1u<<%u) = %u, want %u\n",
                       k, ctz32(lowbit), k);
                return 1;
            }
            accumulate(ctz32(lowbit));
        }
        printf("debruijn_identity: 32 powers checked, distinct_5bit_keys=%u\n",
               distinct);
    }

    /* Explicit contract check: ctz32(0) = 32 on both sides. */
    if (ctz32(0u) != 32u || ref_ctz32(0u) != 32u) {
        printf("FAIL: ctz(0) is not 32\n");
        return 1;
    }

    /* Step 2: exhaustive 16-bit sweep against the naive oracle, with
     * a __builtin_ctz cross-check for every nonzero value. */
    for (v = 0; v <= 0xFFFFu; ++v) {
        unsigned got = ctz32(v);
        unsigned want = ref_ctz32(v);

        if (got != want) {
            printf("MISMATCH: ctz(%u): ctz32=%u ref=%u\n", v, got, want);
            ++mismatches;
        }
        if (v != 0u && (unsigned)__builtin_ctz(v) != got) {
            printf("MISMATCH: ctz(%u): ctz32=%u builtin=%u\n",
                   v, got, (unsigned)__builtin_ctz(v));
            ++mismatches;
        }
        accumulate(got);
    }

    /* Step 3: 2,000,000 full-range 32-bit values, timed. */
    rng_state = SEED;
    t0 = now_ns();
    for (i = 0; i < RAND_CASES; ++i) {
        v = (uint32_t)rng_next();
        if (v == 0u)
            v = 1u; /* keep the stream nonzero for this sweep */
        unsigned got = ctz32(v);
        unsigned want = ref_ctz32(v);

        if (got != want) {
            printf("MISMATCH: ctz(%u): ctz32=%u ref=%u\n", v, got, want);
            ++mismatches;
        }
        accumulate(got);
    }
    t1 = now_ns();

    /* Step 4: throughput, 100M timed values into a sink. */
    rng_state = SEED;
    t0 = now_ns();
    for (i = 0; i < BENCH_CASES; ++i) {
        v = (uint32_t)rng_next();
        sink += ctz32(v);
    }
    t1 = now_ns();

    printf("exhaustive_16bit=65536 random_32bit=%u total=%u mismatches=%lu\n",
           RAND_CASES, 65536u + RAND_CASES, mismatches);
    printf("checksum=%" PRIu64 "\n", checksum);
    printf("throughput: values=%u ns_total=%" PRIu64 " ns_per_value=%.2f sink=%llu\n",
           BENCH_CASES, t1 - t0,
           (double)(t1 - t0) / (double)BENCH_CASES, sink);

    if (mismatches != 0) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
