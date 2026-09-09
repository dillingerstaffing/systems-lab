#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "floor_log2.h"

/* splitmix64, fixed seed: reproducible random 64-bit stream. */
static uint64_t rng_state = 0x123456789ABCDEF0u;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15u);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9u;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBu;
    return z ^ (z >> 31);
}

/* Reference: compiler's clz for nonzero inputs only (__builtin_clzll
 * is undefined at 0, so the differential test never calls it at 0).
 * At 0 the reference is the documented UINT64_MAX convention. */
static uint64_t ref_floor_log2(uint64_t x)
{
    if (x == 0u)
        return UINT64_MAX;
    return 63u - (uint64_t)__builtin_clzll(x);
}

/* FNV-1a over the raw 8 bytes of each result. */
static uint64_t fnv1a = 14695981039346656037u;

static void checksum_add(uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        fnv1a ^= (uint8_t)(v >> (8 * i));
        fnv1a *= 1099511628211u;
    }
}

static uint64_t ns_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

int main(void)
{
    uint64_t mismatches = 0, checked = 0, zeros = 0;

    /* Boundary sweep: 0, 1, and 2^k - 1, 2^k, 2^k + 1 around every
     * bit position, plus UINT64_MAX. */
    {
        uint64_t x;
        x = 0u;
        if (floor_log2(x) != ref_floor_log2(x)) mismatches++;
        checked++;
        for (int k = 0; k < 64; k++) {
            uint64_t v;
            v = (uint64_t)1 << k;                        /* 2^k */
            if (floor_log2(v) != ref_floor_log2(v)) mismatches++;
            checked++;
            checksum_add(floor_log2(v));
            v = ((uint64_t)1 << k) + 1u;                  /* 2^k + 1 */
            if (floor_log2(v) != ref_floor_log2(v)) mismatches++;
            checked++;
            checksum_add(floor_log2(v));
            if (k > 0) {
                v = ((uint64_t)1 << k) - 1u;              /* 2^k - 1 */
                if (floor_log2(v) != ref_floor_log2(v)) mismatches++;
                checked++;
                checksum_add(floor_log2(v));
            }
        }
        x = UINT64_MAX;                                   /* 2^64 - 1 */
        if (floor_log2(x) != ref_floor_log2(x)) mismatches++;
        checked++;
        checksum_add(floor_log2(x));
        checksum_add(floor_log2(0u));
    }

    /* 10M fixed-seed random values, differential against reference. */
    const uint64_t N_RANDOM = 10000000u;
    for (uint64_t i = 0; i < N_RANDOM; i++) {
        uint64_t v = splitmix64();
        if (v == 0u)
            zeros++;
        if (floor_log2(v) != ref_floor_log2(v)) {
            mismatches++;
            if (mismatches < 4)
                printf("MISMATCH v=%" PRIu64 " got=%" PRIu64 " ref=%" PRIu64 "\n",
                       v, floor_log2(v), ref_floor_log2(v));
        }
        checked++;
        checksum_add(floor_log2(v));
    }

    printf("boundaries+random: checked=%" PRIu64 " mismatches=%" PRIu64
           " (zeros_in_random=%" PRIu64 ")\n",
           checked, mismatches, zeros);
    printf("checksum=%" PRIu64 "\n", fnv1a);

    /* Throughput: 100M timed values, PRNG-fed, XORed into a sink so
     * the loop cannot be optimized away. */
    {
        const uint64_t N_TIMED = 100000000u;
        uint64_t sink = 0;
        uint64_t t0 = ns_now();
        for (uint64_t i = 0; i < N_TIMED; i++)
            sink ^= floor_log2(splitmix64());
        uint64_t t1 = ns_now();
        uint64_t ns_total = t1 - t0;
        printf("throughput: values=%" PRIu64 " ns_total=%" PRIu64
               " ns_per_value=%.2f sink=%" PRIu64 "\n",
               N_TIMED, ns_total,
               (double)ns_total / (double)N_TIMED, sink);
    }

    if (mismatches == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("FAIL\n");
    return 1;
}
