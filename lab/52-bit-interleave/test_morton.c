/*
 * lab/52-bit-interleave tests: differential check of the shift/mask
 * Morton code against a naive per-bit-loop reference, plus the
 * deinterleave(interleave(x)) == x round-trip invariant on every case.
 *
 * Phase 1: exhaustive over all 8-bit (lo, hi) pairs (65,536 pairs),
 *   each differential-checked on both interleave and deinterleave.
 * Phase 2: 10,000,000 fixed-seed splitmix64 random 32-bit values, split
 *   into (lo, hi) halves; round-trip plus deinterleave differential.
 * Phase 3: timed loop (round-trip only) for throughput.
 *
 * A 64-bit FNV-1a checksum over the phase 1 and 2 outputs must come out
 * identical in every build; the checksum is printed so the three build
 * logs (-O0, -O2, ASan+UBSan) can be compared.
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "morton.h"

/* fixed seed, so every run and every build sees the same stream */
static uint64_t rng_state = 0x243F6A8885A308D3ull;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint64_t fnv = 0xCBF29CE484222325ull;

static void fnv_mix(uint32_t v)
{
    fnv ^= v;
    fnv *= 0x100000001B3ull;
}

/* naive reference: one bit at a time */
static uint32_t ref_interleave(uint16_t lo, uint16_t hi)
{
    uint32_t z = 0;
    for (int i = 0; i < 16; i++) {
        if (lo & (uint16_t)(1u << i))
            z |= 1u << (2 * i);
        if (hi & (uint16_t)(1u << i))
            z |= 1u << (2 * i + 1);
    }
    return z;
}

/* naive reference: extract each field bit one at a time */
static uint32_t ref_deinterleave(uint32_t z)
{
    uint32_t lo = 0, hi = 0;
    for (int i = 0; i < 16; i++) {
        if (z & (1u << (2 * i)))
            lo |= 1u << i;
        if (z & (1u << (2 * i + 1)))
            hi |= 1u << i;
    }
    return (hi << 16) | lo;
}

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int main(void)
{
    uint64_t checks = 0, mismatches = 0;

    /* phase 1: exhaustive 8-bit pairs */
    for (uint32_t lo = 0; lo < 256; lo++) {
        for (uint32_t hi = 0; hi < 256; hi++) {
            uint32_t z = morton_interleave((uint16_t)lo, (uint16_t)hi);
            if (z != ref_interleave((uint16_t)lo, (uint16_t)hi))
                mismatches++;
            checks++;
            uint32_t packed = morton_deinterleave(z);
            if (packed != ref_deinterleave(z))
                mismatches++;
            checks++;
            if (packed != (((uint32_t)hi << 16) | (uint32_t)lo))
                mismatches++;
            checks++;
            fnv_mix(z);
            fnv_mix(packed);
        }
    }
    printf("phase 1 (exhaustive 8-bit pairs): 65536 pairs, %" PRIu64
           " checks, %" PRIu64 " mismatches\n",
           checks, mismatches);
    uint64_t checks_p1 = checks;

    /* phase 2: 10M fixed-seed random 32-bit values, split into halves */
    for (uint64_t i = 0; i < 10000000ull; i++) {
        uint32_t v = (uint32_t)rng_next();
        uint16_t lo = (uint16_t)v;
        uint16_t hi = (uint16_t)(v >> 16);
        uint32_t z = morton_interleave(lo, hi);
        uint32_t packed = morton_deinterleave(z);
        if (packed != v)
            mismatches++;
        checks++;
        if (packed != ref_deinterleave(z))
            mismatches++;
        checks++;
        fnv_mix(z);
        fnv_mix(packed);
    }
    printf("phase 2 (10M random 32-bit values): %" PRIu64
           " checks, %" PRIu64 " mismatches\n",
           checks - checks_p1, mismatches);

    printf("correctness: %" PRIu64 " differential checks, %" PRIu64
           " mismatches\n",
           checks, mismatches);
    printf("fnv1a checksum: 0x%016" PRIx64 "\n", fnv);

    /* phase 3: timed round-trip loop */
    uint64_t sink = 0;
    uint64_t t0 = now_ns();
    for (uint64_t i = 0; i < 50000000ull; i++) {
        uint32_t v = (uint32_t)rng_next();
        uint32_t z = morton_interleave((uint16_t)v, (uint16_t)(v >> 16));
        sink ^= morton_deinterleave(z);
    }
    uint64_t t1 = now_ns();
    printf("throughput: %.3f ns/pair over 50000000 pairs (sink=0x%016" PRIx64
           ") [loop includes the splitmix64 step, so this is a ceiling]\n",
           (double)(t1 - t0) / 50000000.0, sink);

    if (mismatches == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("FAILURES: %" PRIu64 "\n", mismatches);
    return 1;
}
