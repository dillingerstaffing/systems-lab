#define _POSIX_C_SOURCE 199309L

#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sat_add32.h"

/* splitmix64: deterministic PRNG, fixed seed, reproducible on any build */
static uint64_t rng_state = UINT64_C(0x9E3779B97F4A7C15);

static uint64_t splitmix64(void) {
    uint64_t z = (rng_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* Oracle: the exact sum in 64 bits (cannot overflow for 32-bit
 * operands), clamped to [INT32_MIN, INT32_MAX]. */
static int32_t oracle(int32_t a, int32_t b) {
    int64_t t = (int64_t)a + (int64_t)b;
    if (t > INT32_MAX) {
        return INT32_MAX;
    }
    if (t < INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)t;
}

/* FNV-1a 64-bit over the differential results, so the three build
 * configurations can be checked to compute the identical answers. */
static uint64_t fnv = UINT64_C(0xCBF29CE484222325);

static void fnv_mix(uint32_t v) {
    fnv ^= v;
    fnv *= UINT64_C(0x100000001B3);
}

#define N_RANDOM UINT64_C(1000000)
#define N_EDGE_VALS 256

int main(int argc, char **argv) {
    uint64_t timed_pairs = 200000000ULL;
    if (argc > 1) {
        timed_pairs = strtoull(argv[1], NULL, 10);
    }

    /* Known-answer checks: every corner of the clamp behavior. */
    assert(sat_add32(INT32_MAX, 1) == INT32_MAX);
    assert(sat_add32(INT32_MAX, INT32_MAX) == INT32_MAX);
    assert(sat_add32(INT32_MIN, -1) == INT32_MIN);
    assert(sat_add32(INT32_MIN, INT32_MIN) == INT32_MIN);
    assert(sat_add32(INT32_MAX, 0) == INT32_MAX);
    assert(sat_add32(INT32_MIN, 0) == INT32_MIN);
    assert(sat_add32(0, 0) == 0);
    assert(sat_add32(1, -1) == 0);
    assert(sat_add32(123, 456) == 579);
    assert(sat_add32(1000000000, 2000000000) == INT32_MAX);
    assert(sat_add32(-1000000000, -2000000000) == INT32_MIN);
    printf("phase 0 (known answers): 11 checks, 0 mismatches\n");

    /* Phase 1: 1M fixed-seed random full-range pairs. */
    uint64_t mismatches = 0;
    for (uint64_t i = 0; i < N_RANDOM; i++) {
        int32_t a = (int32_t)(splitmix64() >> 32);
        int32_t b = (int32_t)(splitmix64() >> 32);
        int32_t got = sat_add32(a, b);
        int32_t want = oracle(a, b);
        fnv_mix((uint32_t)got);
        if (got != want) {
            mismatches++;
        }
    }
    printf("phase 1 (1M fixed-seed random pairs): %" PRIu64 " pairs, %" PRIu64
           " mismatches\n",
           N_RANDOM, mismatches);

    /* Phase 2: all 2^16 = 65536 directed edge pairs. The edge set is
     * 256 values: 32 at each of INT32_MIN+i, INT32_MAX-i,
     * -32768+i, 32767-i, plus the 128 values -64..63. Crossed with
     * itself this covers every saturation corner (INT32_MAX+1,
     * INT32_MIN-1, near-boundary carries) in both operand slots. */
    static int32_t edge[N_EDGE_VALS];
    for (int i = 0; i < 32; i++) {
        edge[i] = INT32_MIN + i;
        edge[32 + i] = INT32_MAX - i;
        edge[64 + i] = (int32_t)-32768 + i;
        edge[96 + i] = (int32_t)32767 - i;
    }
    for (int i = 0; i < 128; i++) {
        edge[128 + i] = (int32_t)(-64 + i);
    }
    uint64_t mismatches2 = 0;
    uint64_t edge_pairs = 0;
    for (int i = 0; i < N_EDGE_VALS; i++) {
        for (int j = 0; j < N_EDGE_VALS; j++) {
            int32_t got = sat_add32(edge[i], edge[j]);
            int32_t want = oracle(edge[i], edge[j]);
            fnv_mix((uint32_t)got);
            edge_pairs++;
            if (got != want) {
                mismatches2++;
            }
        }
    }
    printf("phase 2 (directed edge pairs): %" PRIu64 " pairs, %" PRIu64
           " mismatches\n",
           edge_pairs, mismatches2);

    uint64_t total = 11 + N_RANDOM + edge_pairs;
    uint64_t total_mismatches = mismatches + mismatches2;
    printf("correctness: %" PRIu64 " differential checks, %" PRIu64
           " mismatches\n",
           total, total_mismatches);
    printf("checksum (FNV-1a over all results): 0x%016" PRIx64 "\n", fnv);

    /* Phase 3: throughput. Fixed-seed stream, timed, best of 5 runs;
     * results folded into a separate checksum so the optimizer cannot
     * delete the work. */
    double best_ns = 1e30;
    uint64_t timed_check = 0;
    for (int run = 0; run < 5; run++) {
        rng_state = UINT64_C(0x9E3779B97F4A7C15); /* restart the stream */
        splitmix64(); /* advance past the two phases' usage is irrelevant;
                         the stream restarts identically each run */
        uint64_t h = UINT64_C(0xCBF29CE484222325);
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (uint64_t i = 0; i < timed_pairs; i++) {
            int32_t a = (int32_t)(splitmix64() >> 32);
            int32_t b = (int32_t)(splitmix64() >> 32);
            uint32_t r = (uint32_t)sat_add32(a, b);
            h ^= r;
            h *= UINT64_C(0x100000001B3);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                    (double)(t1.tv_nsec - t0.tv_nsec);
        if (ns < best_ns) {
            best_ns = ns;
            timed_check = h;
        }
    }
    printf("throughput: %.3f ns/pair over %" PRIu64
           " timed pairs (best of 5, timed checksum=0x%016" PRIx64 ")\n",
           best_ns / (double)timed_pairs, timed_pairs, timed_check);

    if (total_mismatches != 0) {
        printf("FAILED\n");
        return 1;
    }
    printf("ALL TESTS PASSED\n");
    return 0;
}
