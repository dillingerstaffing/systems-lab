/*
 * Differential test for onesc_add: 16-bit one's-complement
 * addition with the wraparound carry folded back in.
 *
 * Oracle: a structurally different loop-fold reference (the RFC
 * 1071 form). It sums in 32 bits, then repeatedly folds the high
 * 16 bits down until nothing is left: while (sum >> 16)
 * sum = (sum & 0xFFFF) + (sum >> 16). This shares no arithmetic
 * with the tested implementation (no 16-bit wraparound, no
 * comparison-derived carry), so agreement is genuine evidence.
 *
 * Verification plan:
 *
 *   1. Edge pins: 0xFFFF + 0xFFFF must be 0xFFFF (the all-ones
 *      one's-complement zero identity), plus 0 + 0, 0xFFFF + 0,
 *      and 0x8000 + 0x8000 == 0x0001 (carry folded in).
 *   2. 65,536 directed pairs: a sweeps 0..65535 with b = 0xFFFF,
 *      forcing the wraparound carry on every pair (a + 0xFFFF
 *      wraps whenever a > 0).
 *   3. 10,000,000 pairs from a fixed-seed splitmix64 generator
 *      (seed 0x123456789ABCDEF0, fully reproducible), each pair
 *      compared against the loop-fold oracle.
 *   4. A 64-bit FNV-1a checksum accumulates every verified result
 *      and must be identical across the -O0, -O2, and
 *      ASan+UBSan builds.
 *   5. Throughput: 100,000,000 timed pairs at whatever build flags
 *      are in use, best of 5 runs, results accumulated into a sink
 *      so the loop cannot be optimized away.
 */
#include "onesc_add.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define RAND_CASES 10000000u
#define BENCH_CASES 100000000u
#define BENCH_ROUNDS 5u

/* Fixed seed, stated here so the random stream is reproducible. */
#define SEED 0x123456789ABCDEF0ULL

static uint64_t rng_state;

/* splitmix64: the full 64-bit mix, a != b drawn as two 16-bit
 * slices so the low bits of the generator still vary. */
static uint64_t rng_next(void)
{
    uint64_t z = rng_state += 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Loop-fold oracle: accumulate in 32 bits, fold the carry down
 * until it fits in 16. No wraparound comparison anywhere. */
static uint16_t ref_onesc_add(uint16_t a, uint16_t b)
{
    uint32_t sum = (uint32_t)a + (uint32_t)b;

    while (sum >> 16)
        sum = (sum & 0xFFFFu) + (sum >> 16);
    return (uint16_t)sum;
}

static uint64_t checksum;

static void accumulate(uint16_t v)
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
    unsigned long long total = 0;
    uint32_t a;
    uint64_t i, r;
    uint64_t t0, t1;
    uint64_t best_ns = 0;
    unsigned long long sink = 0;

    checksum = 14695981039346656037ULL; /* FNV-1a offset basis */

    /* Step 1: pin the edges, starting with the all-ones identity. */
    {
        static const uint16_t ea[] = { 0xFFFFu, 0u, 0xFFFFu, 0x8000u };
        static const uint16_t eb[] = { 0xFFFFu, 0u, 0u, 0x8000u };
        static const uint16_t want[] = { 0xFFFFu, 0u, 0xFFFFu, 0x0001u };
        unsigned k;

        for (k = 0; k < 4; ++k) {
            uint16_t got = onesc_add(ea[k], eb[k]);

            if (got != want[k] || got != ref_onesc_add(ea[k], eb[k])) {
                printf("FAIL: edge %u: onesc_add(0x%04X, 0x%04X) = 0x%04X, want 0x%04X\n",
                       k, ea[k], eb[k], got, want[k]);
                return 1;
            }
            accumulate(got);
        }
        printf("edge_pins: 4 checked, onesc_add(0xFFFF, 0xFFFF) = 0x%04X\n",
               onesc_add(0xFFFFu, 0xFFFFu));
    }

    /* Step 2: 65,536 directed pairs with b = 0xFFFF. Every nonzero
     * a forces the wraparound carry, so the (s < a) recovery is
     * exercised on every case. */
    for (a = 0; a <= 0xFFFFu; ++a) {
        uint16_t ua = (uint16_t)a;
        uint16_t got = onesc_add(ua, 0xFFFFu);
        uint16_t want = ref_onesc_add(ua, 0xFFFFu);

        if (got != want) {
            printf("MISMATCH: onesc_add(0x%04X, 0xFFFF) = 0x%04X, ref 0x%04X\n",
                   ua, got, want);
            ++mismatches;
        }
        accumulate(got);
        ++total;
    }

    /* Step 3: 10,000,000 fixed-seed random pairs against the oracle. */
    rng_state = SEED;
    for (i = 0; i < RAND_CASES; ++i) {
        r = rng_next();
        uint16_t ua = (uint16_t)r;
        uint16_t ub = (uint16_t)(r >> 32);
        uint16_t got = onesc_add(ua, ub);
        uint16_t want = ref_onesc_add(ua, ub);

        if (got != want) {
            printf("MISMATCH: onesc_add(0x%04X, 0x%04X) = 0x%04X, ref 0x%04X\n",
                   ua, ub, got, want);
            ++mismatches;
        }
        accumulate(got);
        ++total;
    }

    /* Step 4: throughput, 100M pairs, best of 5 rounds. */
    for (r = 0; r < BENCH_ROUNDS; ++r) {
        rng_state = SEED;
        t0 = now_ns();
        for (i = 0; i < BENCH_CASES; ++i) {
            uint64_t x = rng_next();
            sink += onesc_add((uint16_t)x, (uint16_t)(x >> 32));
        }
        t1 = now_ns();
        if (r == 0 || t1 - t0 < best_ns)
            best_ns = t1 - t0;
    }

    printf("directed_65536=65536 random_pairs=%u total=%llu mismatches=%lu\n",
           RAND_CASES, total, mismatches);
    printf("checksum=%" PRIu64 "\n", checksum);
    printf("throughput: pairs=%u best_of_5 ns_total=%" PRIu64 " ns_per_pair=%.2f sink=%llu\n",
           BENCH_CASES, best_ns,
           (double)best_ns / (double)BENCH_CASES, sink);

    if (mismatches != 0) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
