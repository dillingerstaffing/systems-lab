#include <stdint.h>
#include <stdio.h>

#include "bitperm.h"

/* Differential test: perm8/invperm8 against naive per-bit loop references.
 *
 *  - mapping self-check: each single-bit input lands on the exact bit named
 *    by P (resp. Q); P holds each of 0..7 exactly once; Q[P[i]] = i and
 *    P[Q[i]] = i for every i
 *  - exhaustive: all 65536 16-bit inputs (perm applied to both bytes);
 *    perm vs naive, inv vs naive, inv(perm(x)) == x, perm(inv(x)) == x
 *  - 1,000,000 fixed-seed splitmix64 64-bit values (seed 0x123456789ABCDEF0,
 *    perm applied to all 8 bytes); same four checks per value
 *
 * An FNV-1a 64-bit checksum is folded over every perm/inv result, so the
 * three builds (-O0, -O2, ASan+UBSan) can be cross-checked for bit-identical
 * outputs.
 */

static const uint8_t P[8] = {2, 5, 0, 7, 1, 6, 3, 4};
static const uint8_t Q[8] = {2, 4, 0, 6, 7, 1, 5, 3};

/* Naive per-bit loop references: the ground truth for the differential
 * test. Kept deliberately separate from bitperm.h. */

static uint8_t perm8_ref(uint8_t b)
{
    uint8_t y = 0;
    int i;
    for (i = 0; i < 8; i++)
        if ((b >> i) & 1u)
            y |= (uint8_t)(1u << P[i]);
    return y;
}

static uint8_t invperm8_ref(uint8_t b)
{
    uint8_t y = 0;
    int i;
    for (i = 0; i < 8; i++)
        if ((b >> i) & 1u)
            y |= (uint8_t)(1u << Q[i]);
    return y;
}

static uint16_t perm16_ref(uint16_t x)
{
    uint16_t y = 0;
    int i;
    for (i = 0; i < 16; i++)
        if ((x >> i) & 1u)
            y |= (uint16_t)(1u << (P[i & 7] + 8 * (i >> 3)));
    return y;
}

static uint16_t invperm16_ref(uint16_t x)
{
    uint16_t y = 0;
    int i;
    for (i = 0; i < 16; i++)
        if ((x >> i) & 1u)
            y |= (uint16_t)(1u << (Q[i & 7] + 8 * (i >> 3)));
    return y;
}

static uint64_t perm64_ref(uint64_t x)
{
    uint64_t y = 0;
    int i;
    for (i = 0; i < 64; i++)
        if ((x >> i) & 1ull)
            y |= 1ull << (P[i & 7] + 8 * (i >> 3));
    return y;
}

static uint64_t invperm64_ref(uint64_t x)
{
    uint64_t y = 0;
    int i;
    for (i = 0; i < 64; i++)
        if ((x >> i) & 1ull)
            y |= 1ull << (Q[i & 7] + 8 * (i >> 3));
    return y;
}

static uint64_t rng_state = 0x123456789ABCDEF0ull;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 14695981039346656037ull;

static void fnv_mix64(uint64_t v)
{
    int b;
    for (b = 0; b < 8; b++) {
        fnv1a ^= (v >> (8 * b)) & 0xFFull;
        fnv1a *= 1099511628211ull;
    }
}

static uint64_t mismatches;
static uint64_t checks;

#define CHECK(cond, fmt, ...) do { \
    checks++; \
    if (!(cond)) { \
        if (mismatches < 8) \
            printf("MISMATCH " fmt "\n", ##__VA_ARGS__); \
        mismatches++; \
    } \
} while (0)

int main(void)
{
    int i;
    uint64_t n;
    uint32_t seen;

    /* mapping self-check: single-bit placement, permutation-ness of P,
     * and the exact index-reversal relation between P and Q */
    seen = 0;
    for (i = 0; i < 8; i++) {
        CHECK(perm8((uint8_t)(1u << i)) == (uint8_t)(1u << P[i]),
              "perm8 bit %d", i);
        CHECK(invperm8((uint8_t)(1u << i)) == (uint8_t)(1u << Q[i]),
              "invperm8 bit %d", i);
        seen |= 1u << P[i];
        CHECK(Q[P[i]] == (uint8_t)i, "Q[P[%d]] != %d", i, i);
        CHECK(P[Q[i]] == (uint8_t)i, "P[Q[%d]] != %d", i, i);
    }
    CHECK(seen == 0xFFu, "P is not a permutation of 0..7");
    printf("mapping self-check done: 33 checks\n");

    /* exhaustive 16-bit: four differential assertions per input */
    for (n = 0; n < 65536ull; n++) {
        uint16_t x = (uint16_t)n;
        uint16_t p = perm16(x);
        uint16_t q = invperm16(x);
        CHECK(p == perm16_ref(x), "perm16 x=0x%04x got=0x%04x ref=0x%04x",
              x, p, perm16_ref(x));
        CHECK(q == invperm16_ref(x), "invperm16 x=0x%04x", x);
        CHECK((p & 0xFFu) == perm8_ref((uint8_t)(x & 0xFFu)),
              "perm8 lo byte x=0x%04x", x);
        CHECK((p >> 8) == perm8_ref((uint8_t)(x >> 8)),
              "perm8 hi byte x=0x%04x", x);
        CHECK((q & 0xFFu) == invperm8_ref((uint8_t)(x & 0xFFu)),
              "invperm8 lo byte x=0x%04x", x);
        CHECK((q >> 8) == invperm8_ref((uint8_t)(x >> 8)),
              "invperm8 hi byte x=0x%04x", x);
        CHECK(invperm16(p) == x, "inv(perm(x)) x=0x%04x", x);
        CHECK(perm16(q) == x, "perm(inv(x)) x=0x%04x", x);
        fnv_mix64(p);
        fnv_mix64(q);
    }
    printf("exhaustive 16-bit done: 65536 values\n");

    /* 1M fixed-seed 64-bit values, permuted byte by byte */
    rng_state = 0x123456789ABCDEF0ull;
    for (n = 0; n < 1000000ull; n++) {
        uint64_t x = splitmix64();
        uint64_t p = perm64(x);
        uint64_t q = invperm64(x);
        CHECK(p == perm64_ref(x), "perm64 x=0x%016llx",
              (unsigned long long)x);
        CHECK(q == invperm64_ref(x), "invperm64 x=0x%016llx",
              (unsigned long long)x);
        CHECK(invperm64(p) == x, "inv(perm(x)) x=0x%016llx",
              (unsigned long long)x);
        CHECK(perm64(q) == x, "perm(inv(x)) x=0x%016llx",
              (unsigned long long)x);
        fnv_mix64(p);
        fnv_mix64(q);
    }
    printf("random 64-bit done: 1000000 values, splitmix64 seed 0x123456789ABCDEF0\n");

    printf("total checks            : %llu\n", (unsigned long long)checks);
    printf("mismatches              : %llu\n", (unsigned long long)mismatches);
    printf("FNV-1a of all results   : 0x%016llx\n", (unsigned long long)fnv1a);
    return mismatches == 0 ? 0 : 1;
}
