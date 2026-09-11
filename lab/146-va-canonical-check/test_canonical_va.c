#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "canonical_va.h"

/* Fixed-seed splitmix64, the same constants every run, every build. */
static uint64_t rng_state;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/*
 * Oracle: the rule stated per bit, the way the architecture manual
 * states it. Bit 38 is the sign bit of the 39-bit address; each of
 * bits 63..39 must individually equal it. Written as 25 separate
 * single-bit comparisons so it shares no structure with the field
 * shift in canonical_va (no whole-field compare anywhere in the
 * oracle).
 */
static int oracle_canonical(uint64_t va)
{
    uint64_t b38 = (va >> 38) & 1u;
    for (unsigned i = 39; i < 64; i++) {
        if (((va >> i) & 1u) != b38)
            return 0;
    }
    return 1;
}

/* FNV-1a 64-bit, folded over every result so the checksum must match
 * across -O0, -O2, and sanitizer builds. */
static uint64_t fnv = 14695981039346656037ULL;

static void feed(int r)
{
    fnv ^= (uint64_t)(uint32_t)r;
    fnv *= 1099511628211ULL;
}

static uint64_t checks;
static uint64_t mismatches;
static uint64_t directed_count;

static uint64_t directed[80];
static size_t n_directed;

static void add_directed(uint64_t v)
{
    size_t i;
    for (i = 0; i < n_directed; i++)
        if (directed[i] == v)
            return;
    directed[n_directed++] = v;
}

static void check_case(uint64_t va)
{
    int got = canonical_va(va);
    int want = oracle_canonical(va);

    checks++;
    if (got != want) {
        mismatches++;
        printf("MISMATCH oracle: va=0x%016" PRIx64 " got=%d want=%d\n",
               va, got, want);
    }
    feed(got);

    /* Invariant 1: the verdict is decided by bits 63:38 only, never by
     * the low 38. Replace bits 37:0 with a random 38-bit value and the
     * verdict must not change. (Bit 38 is part of the verdict: it is
     * the bit the top 25 must match, so it is not randomized.) */
    {
        uint64_t va2 = (va & ~0x3FFFFFFFFFULL)
                     | (splitmix64() & 0x3FFFFFFFFFULL);
        int got2 = canonical_va(va2);

        checks++;
        if (got2 != got) {
            mismatches++;
            printf("MISMATCH lowbits: va=0x%016" PRIx64
                   " va2=0x%016" PRIx64 " got=%d got2=%d\n",
                   va, va2, got, got2);
        }
        feed(got2);
    }

    /* Invariant 2: sign-extension round trip. For a canonical va,
     * sign-extending the 39-bit address reproduces va exactly; for a
     * non-canonical va it cannot. The right shift must be the
     * arithmetic shift of the signed value, so the cast happens before
     * it (gcc performs signed >> as arithmetic; shifting the unsigned
     * value first would be a logical shift and the model would be
     * wrong). */
    {
        int64_t ext = ((int64_t)(va << 25)) >> 25;
        int roundtrip = (ext == (int64_t)va);

        checks++;
        if (roundtrip != (got != 0)) {
            mismatches++;
            printf("MISMATCH roundtrip: va=0x%016" PRIx64
                   " got=%d roundtrip=%d\n",
                   va, got, roundtrip);
        }
        feed(roundtrip);
    }
}

int main(void)
{
    unsigned i;

    rng_state = 0x123456789ABCDEF0ULL;

    /* Directed sign-boundary rows. */
    add_directed(0ULL);
    add_directed((1ULL << 38) - 1);
    add_directed(1ULL << 38);
    add_directed((1ULL << 38) + 1);
    add_directed((1ULL << 39) - 1);
    add_directed(1ULL << 39);
    for (i = 39; i < 64; i++) {
        add_directed(1ULL << i);              /* single high bit, b38 = 0 */
        add_directed((1ULL << i) | (1ULL << 38)); /* single high bit, b38 = 1 */
    }
    add_directed(0x0000007FFFFFFFFFULL);
    add_directed(0xFFFFFF8000000000ULL);
    add_directed(UINT64_MAX);
    directed_count = n_directed;

    for (i = 0; i < n_directed; i++)
        check_case(directed[i]);

    /* 10,000,000 fixed-seed splitmix64 64-bit values. */
    {
        uint64_t n = 10000000ULL;
        for (uint64_t k = 0; k < n; k++)
            check_case(splitmix64());
    }

    /* Machine-readable result block, copied verbatim into PROOF.md. */
    printf("HEADER-BEGIN\n");
    printf("Checks: %" PRIu64 "\n", checks);
    printf("Mismatches: %" PRIu64 "\n", mismatches);
    printf("Checksum: %016" PRIx64 "\n", fnv);
    printf("HEADER-END\n");
    printf("directed_cases: %" PRIu64 "\n", directed_count);
    printf("verdict: %s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
