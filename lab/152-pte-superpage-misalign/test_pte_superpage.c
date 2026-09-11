/*
 * Differential test for pte_superpage_misaligned().
 *
 * The implementation builds a low-bit mask ((1ULL << 9*level) - 1) and
 * tests the masked PPN. The oracle below is structurally different: it
 * scans the 44-bit PPN field one bit at a time from the top down and
 * reports misalignment the moment it finds a set bit whose position
 * lies below 9*level. No mask is ever constructed, and the loop runs
 * over positions rather than applying one bitwise expression.
 */
#include <stdio.h>
#include <stdint.h>

#include "pte_superpage.h"

#define PPN_ALL44 0xFFFFFFFFFFFULL /* 44 one-bits */

/* ---------------- independent oracle ---------------- */

static int oracle_misaligned(int level, uint64_t ppn)
{
    uint64_t p = ppn & PPN_ALL44;
    int b;

    if (level < 0 || level > 2)
        return -1;
    if (level == 0)
        return 0;
    /* A set bit at position b < 9*level sits inside the superpage's
     * span and makes the PPN misaligned. */
    for (b = 43; b >= 0; b--) {
        if (b < 9 * level && ((p >> b) & 1ULL) != 0)
            return 1;
    }
    return 0;
}

/* ---------------- fixed-seed PRNG (splitmix64) ---------------- */

static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* ---------------- FNV-1a checksum over verdict bytes ---------------- */

static uint64_t fnv = 0xCBF29CE484222325ULL;

static void fnv_feed(unsigned char byte)
{
    fnv ^= byte;
    fnv *= 0x100000001B3ULL;
}

/* ---------------- check harness ---------------- */

static unsigned long long nchecks;
static unsigned long long nmismatches;

static void check(int level, uint64_t ppn)
{
    int got = pte_superpage_misaligned(level, ppn);
    int want = oracle_misaligned(level, ppn);

    nchecks++;
    /* Verdict bytes are -1, 0, or 1; shift to 0, 1, 2 for the hash. */
    fnv_feed((unsigned char)(got + 1));
    if (got != want) {
        nmismatches++;
        if (nmismatches <= 10)
            printf("MISMATCH level=%d ppn=0x%011llx got=%d want=%d\n",
                   level, (unsigned long long)(ppn & PPN_ALL44),
                   got, want);
    }
}

int main(void)
{
    static const int ood_levels[] = { -1, 3, 4, 100, -100 };
    int level, b;
    size_t i;

    /* Directed rows: PPN=0, PPN all-ones, low bits all set, low bits
     * all clear with high bits set, boundary bit at 9*level, and a
     * single-bit sweep across every position that can matter. */
    for (level = 0; level <= 2; level++) {
        uint64_t lowmask = (level == 0) ? 0ULL
                                        : ((1ULL << (9 * level)) - 1ULL);
        int sweep_hi = (level == 2) ? 18 : 9;

        check(level, 0ULL);                 /* (e) PPN = 0 */
        check(level, PPN_ALL44);            /* (d) all 44 bits set */
        check(level, lowmask);              /* every low bit set */
        check(level, PPN_ALL44 ^ lowmask);  /* low bits clear, rest set */
        if (level > 0)
            /* boundary bit exactly at 9*level plus a high bit:
             * must NOT fault. */
            check(level, (1ULL << (9 * level)) | (1ULL << 43));

        /* (b) single bit at each position: faults below 9*level,
         * clean at and above it; (f) level 0 never faults. */
        for (b = 0; b <= sweep_hi; b++)
            check(level, 1ULL << b);
    }

    /* Out-of-domain levels: both sides must report -1 explicitly. */
    for (i = 0; i < sizeof(ood_levels) / sizeof(ood_levels[0]); i++) {
        check(ood_levels[i], 0ULL);
        check(ood_levels[i], PPN_ALL44);
    }

    /* 10,000,000 fixed-seed random 44-bit PPNs per level. */
    for (level = 0; level <= 2; level++) {
        unsigned long long n;

        rng_state = 0x123456789ABCDEF0ULL;
        for (n = 0; n < 10000000ULL; n++)
            check(level, rng_next() & PPN_ALL44);
    }

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           nchecks, nmismatches, (unsigned long long)fnv);
    return nmismatches == 0 ? 0 : 1;
}
