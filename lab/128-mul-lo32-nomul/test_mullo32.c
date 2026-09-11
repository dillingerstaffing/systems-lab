/*
 * Differential test for mullo32 (lab/128-mul-lo32-nomul).
 *
 * Phase 1: a runs exhaustively over all 2^24 values; b is a deterministic
 * avalanche (one splitmix64 finalizer pass) of a. 16,777,216 pairs.
 * Phase 2: 1,000,000 pairs of full 64-bit values from splitmix64 with the
 * fixed seed 0x123456789ABCDEF0. Total: 17,777,216 checks.
 *
 * Oracle: the native C expression (uint32_t)(a * b). This test harness may
 * use the multiply operator freely; the implementation under test
 * (mul32.c) may not, and that is enforced separately by `make disasm`,
 * which scans the -O2 object code for multiply-class instructions.
 *
 * Every implementation output is folded into a 64-bit FNV-1a checksum
 * (offset basis 14695981039346656037, prime 1099511628211, 4 bytes per
 * output word, least significant byte first). The checksum must be
 * identical across the -O0, -O2, and ASan+UBSan builds.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include "mul32.h"

static uint64_t rng_state;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* Deterministic 24-bit avalanche of a 24-bit input, harness only. */
static uint32_t avalanche24(uint32_t x)
{
    uint64_t z = (uint64_t)x + 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return (uint32_t)((z ^ (z >> 31)) & 0xFFFFFFull);
}

static uint64_t fnv = 14695981039346656037ull;

static void fnv_fold(uint32_t w)
{
    for (int i = 0; i < 4; i++) {
        fnv ^= (uint64_t)((w >> (8 * i)) & 0xFFu);
        fnv *= 1099511628211ull;
    }
}

static uint64_t checks;
static uint64_t mismatches;

static void check(uint64_t a, uint64_t b)
{
    uint32_t got = mullo32(a, b);
    uint32_t want = (uint32_t)(a * b);
    if (got != want) {
        if (mismatches < 8)
            printf("MISMATCH a=%016" PRIx64 " b=%016" PRIx64
                   " got=%08" PRIx32 " want=%08" PRIx32 "\n",
                   a, b, got, want);
        mismatches++;
    }
    fnv_fold(got);
    checks++;
}

int main(void)
{
    for (uint64_t i = 0; i < 16777216ull; i++)
        check(i, avalanche24((uint32_t)i));
    printf("phase1 (exhaustive 24-bit a): checks=%" PRIu64
           " mismatches=%" PRIu64 "\n",
           checks, mismatches);

    rng_state = 0x123456789ABCDEF0ull;
    for (uint64_t i = 0; i < 1000000ull; i++)
        check(splitmix64(), splitmix64());
    printf("phase2 (1M random 64-bit pairs): checks=%" PRIu64
           " mismatches=%" PRIu64 "\n",
           checks, mismatches);

    printf("total checks=%" PRIu64 " mismatches=%" PRIu64
           " checksum=%016" PRIx64 "\n",
           checks, mismatches, fnv);
    printf("VERDICT: %s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
