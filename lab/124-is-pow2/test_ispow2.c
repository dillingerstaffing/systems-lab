#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "ispow2.h"

/* splitmix64, fixed seed. Plain generator code, written out by hand. */
static uint64_t sm_state;

static uint64_t splitmix64(void) {
    uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Oracle: per-bit popcount loop, no builtins. Exactly one set bit. */
static int oracle_pow2(uint64_t x) {
    int bits = 0;
    for (int k = 0; k < 64; k++) {
        bits += (int)((x >> k) & 1ULL);
    }
    return bits == 1;
}

/* FNV-1a 64 over one byte. */
static uint64_t fnv1a_step(uint64_t h, uint8_t b) {
    h ^= b;
    h *= 0x100000001B3ULL;
    return h;
}

int main(void) {
    uint64_t checks = 0, mismatches = 0;
    uint64_t hash = 0xCBF29CE484222325ULL; /* FNV offset basis */

    /* (a) Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..63. */
    for (int k = 0; k < 64; k++) {
        uint64_t vals[3] = { 1ULL << k, (1ULL << k) - 1ULL, (1ULL << k) + 1ULL };
        for (int i = 0; i < 3; i++) {
            int got = is_pow2(vals[i]);
            int want = oracle_pow2(vals[i]);
            hash = fnv1a_step(hash, (uint8_t)got);
            checks++;
            if (got != want) {
                mismatches++;
                printf("MISMATCH k=%d x=%llu got=%d want=%d\n",
                       k, (unsigned long long)vals[i], got, want);
            }
        }
    }

    /* (b) Exhaustive 24-bit sweep: all x in [0, 2^24). */
    for (uint32_t x = 0; x < (1U << 24); x++) {
        int got = is_pow2((uint64_t)x);
        int want = oracle_pow2((uint64_t)x);
        hash = fnv1a_step(hash, (uint8_t)got);
        checks++;
        if (got != want) {
            mismatches++;
            if (mismatches < 10)
                printf("MISMATCH x=%u got=%d want=%d\n", x, got, want);
        }
    }

    /* (c) 10M fixed-seed splitmix64 64-bit values. */
    sm_state = 0x123456789ABCDEF0ULL;
    for (uint64_t i = 0; i < 10000000ULL; i++) {
        uint64_t x = splitmix64();
        int got = is_pow2(x);
        int want = oracle_pow2(x);
        hash = fnv1a_step(hash, (uint8_t)got);
        checks++;
        if (got != want) {
            mismatches++;
            if (mismatches < 10)
                printf("MISMATCH x=%llu got=%d want=%d\n",
                       (unsigned long long)x, got, want);
        }
    }

    /* Explicit x = 0 contract: must return 0. */
    if (is_pow2(0) != 0) {
        printf("CONTRACT VIOLATION: is_pow2(0) != 0\n");
        mismatches++;
    }

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           (unsigned long long)checks, (unsigned long long)mismatches,
           (unsigned long long)hash);
    return mismatches == 0 ? 0 : 1;
}
