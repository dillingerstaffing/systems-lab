#include <stdio.h>
#include <stdint.h>
#include "pte_phys.h"

/* Field contracts: PPN is 44 bits, page offset is 12 bits. */
#define PPN_MASK 0xFFFFFFFFFFFULL
#define OFF_MASK 0xFFFULL

/* splitmix64, fixed seed. Plain generator code, written out by hand. */
static uint64_t sm_state;

static uint64_t splitmix64(void) {
    uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/*
 * Oracle: structurally independent of the shift/OR implementation.
 * The implementation moves whole fields: (ppn & mask) << 12 then
 * OR. The oracle never shifts a whole field: it walks destination
 * bit indices 0..55 and sets each destination bit individually from
 * one single-bit extraction of the source input, then checks the
 * two results agree. No helper code is shared with pte_phys.c.
 */
static uint64_t oracle_phys_addr(uint64_t ppn, uint64_t offset) {
    uint64_t phys = 0;
    int b;
    for (b = 0; b < 56; b++) {
        uint64_t src = (b < 12) ? offset : ppn;
        int sb = (b < 12) ? b : (b - 12);
        phys |= ((src >> sb) & 1ULL) << b;
    }
    return phys;
}

/* FNV-1a 64 over one byte. */
static uint64_t fnv1a_step(uint64_t h, uint8_t b) {
    h ^= b;
    h *= 0x100000001B3ULL;
    return h;
}

static uint64_t checks = 0, mismatches = 0;
static uint64_t hash = 0xCBF29CE484222325ULL; /* FNV offset basis */

/*
 * One differential case: the implementation and the per-bit oracle
 * must produce the same 56-bit physical address. The checksum folds
 * the output bytes (7 bytes cover bits 55:0), so the run's output
 * bytes are pinned, not just the mismatch count.
 */
static void check_pair(uint64_t ppn, uint64_t offset) {
    uint64_t got = phys_addr_from_ppn(ppn, offset);
    uint64_t want = oracle_phys_addr(ppn, offset);
    int b;

    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH ppn=%llx offset=%llx got=%llx want=%llx\n",
                   (unsigned long long)ppn, (unsigned long long)offset,
                   (unsigned long long)got, (unsigned long long)want);
    }
    for (b = 0; b < 7; b++)
        hash = fnv1a_step(hash, (uint8_t)((got >> (8 * b)) & 0xFFULL));
    checks++;
}

int main(void) {
    int k;
    uint64_t i, v;

    /* (a) Directed edges over the 56-bit physical space: 2^k,
     * 2^k - 1, 2^k + 1 for k = 0..55, mapped to (ppn, offset)
     * pairs by splitting at bit 12. */
    for (k = 0; k < 56; k++) {
        uint64_t vals[3] = { 1ULL << k, (1ULL << k) - 1ULL,
                             (1ULL << k) + 1ULL };
        for (i = 0; i < 3; i++)
            check_pair((vals[i] >> 12) & PPN_MASK, vals[i] & OFF_MASK);
    }

    /* (b) All-ones / all-zeros rows. */
    check_pair(0ULL, 0ULL);
    check_pair(PPN_MASK, OFF_MASK);
    check_pair(PPN_MASK, 0ULL);
    check_pair(0ULL, OFF_MASK);

    /* (c) Mask-contract rows: bits above the 44/12-bit contracts
     * must be ignored, never read. */
    check_pair(0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL);
    check_pair(0x0000100000000000ULL, 0ULL); /* bit 44 set */
    check_pair(0ULL, 0xFFFFFFFFFFFFF000ULL); /* offset bit 12 set */
    check_pair(0xFFF0000000000000ULL, 0xFFF0000000000000ULL);

    /* (d) Exhaustive 24-bit PPN lanes, three lanes:
     * low lane with offset 0x000, low lane with offset 0xFFF
     * (offset/PPN boundary fully exercised), and the top lane
     * ppn in [2^44 - 2^24, 2^44) with offset 0x5A5. */
    for (v = 0; v < (1ULL << 24); v++)
        check_pair(v, 0ULL);
    for (v = 0; v < (1ULL << 24); v++)
        check_pair(v, OFF_MASK);
    for (v = 0; v < (1ULL << 24); v++)
        check_pair(0xFFFFF000000ULL + v, 0x5A5ULL);

    /* (e) 10M fixed-seed splitmix64 (ppn, offset) pairs, full
     * 64-bit inputs so the mask contract is exercised hard. */
    sm_state = 0x123456789ABCDEF0ULL;
    for (i = 0; i < 10000000ULL; i++)
        check_pair(splitmix64(), splitmix64());

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           (unsigned long long)checks, (unsigned long long)mismatches,
           (unsigned long long)hash);

    /* Machine-readable header lines: the PROOF-HEADER block in
     * PROOF.md is stamped from these values at run time. */
    printf("header-checks=%llu\n", (unsigned long long)checks);
    printf("header-mismatches=%llu\n", (unsigned long long)mismatches);
    printf("header-checksum=%016llx\n", (unsigned long long)hash);

    return mismatches == 0 ? 0 : 1;
}
