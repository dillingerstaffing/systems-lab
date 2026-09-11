#include <stdio.h>
#include <stdint.h>
#include "pte_decode.h"

/* Bits the decoder reads: 53:0. Bits 63:54 are reserved and ignored. */
#define PTE_FIELD_MASK 0x003FFFFFFFFFFFFFULL
#define PTE_RESV_MASK  0xFFC0000000000000ULL

/* splitmix64, fixed seed. Plain generator code, written out by hand. */
static uint64_t sm_state;

static uint64_t splitmix64(void) {
    uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/*
 * Oracle: structurally different from the shift/mask implementation.
 * One loop per field; each loop sets its destination field bit-by-bit
 * from single-bit extractions of the input word, never shifting or
 * masking a whole multi-bit field at once. The eight single-bit flags
 * go through a destination pointer table so no per-field shift/mask
 * code is shared with the implementation. Each bit is assigned (not
 * branched on), so the loop is branchless; the structural difference
 * from the implementation is preserved.
 */
static pte_fields_t oracle_decode(uint64_t pte) {
    pte_fields_t f;
    uint8_t *flag_dst[8];
    int b, i;

    f.ppn = 0;
    f.rsw = 0;
    f.v = 0; f.r = 0; f.w = 0; f.x = 0;
    f.u = 0; f.g = 0; f.a = 0; f.d = 0;

    flag_dst[0] = &f.v; flag_dst[1] = &f.r;
    flag_dst[2] = &f.w; flag_dst[3] = &f.x;
    flag_dst[4] = &f.u; flag_dst[5] = &f.g;
    flag_dst[6] = &f.a; flag_dst[7] = &f.d;

    for (i = 0; i < 8; i++)
        *flag_dst[i] = (uint8_t)((pte >> i) & 1ULL);
    for (b = 0; b < 2; b++)
        f.rsw = (uint8_t)(f.rsw |
                          (uint8_t)(((pte >> (8 + b)) & 1ULL) << b));
    for (b = 0; b < 44; b++)
        f.ppn |= ((pte >> (10 + b)) & 1ULL) << b;
    return f;
}

static int fields_equal(pte_fields_t x, pte_fields_t y) {
    return x.ppn == y.ppn && x.rsw == y.rsw &&
           x.v == y.v && x.r == y.r && x.w == y.w && x.x == y.x &&
           x.u == y.u && x.g == y.g && x.a == y.a && x.d == y.d;
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
 * One differential case: decoded fields must match the oracle, and
 * re-encoding the decoded fields must restore exactly the bits the
 * decoder reads (53:0). For every 32-bit corpus value the mask is a
 * no-op, so there the check is literally encode(decode(pte)) == pte.
 */
static void check_pte(uint64_t pte) {
    pte_fields_t got = pte_decode(pte);
    pte_fields_t want = oracle_decode(pte);
    int ok = fields_equal(got, want) &&
             (pte_encode(got) == (pte & PTE_FIELD_MASK));

    /* Checksum folds the decoded output: ppn low byte first (6 bytes
     * cover 44 bits), then rsw, then the packed flag byte. */
    {
        uint64_t ppn = got.ppn;
        int b;
        for (b = 0; b < 6; b++) {
            hash = fnv1a_step(hash, (uint8_t)(ppn & 0xFFULL));
            ppn >>= 8;
        }
        hash = fnv1a_step(hash, got.rsw);
        hash = fnv1a_step(hash, (uint8_t)((got.d << 7) | (got.a << 6) |
                                         (got.g << 5) | (got.u << 4) |
                                         (got.x << 3) | (got.w << 2) |
                                         (got.r << 1) | got.v));
    }

    checks++;
    if (!ok) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH pte=%llx\n", (unsigned long long)pte);
    }
}

/*
 * Reserved-bits row: setting bits 63:54 must not change any decoded
 * field. This pins the input contract that the decoder never reads
 * the reserved top bits.
 */
static void check_reserved(uint64_t base) {
    pte_fields_t plain = pte_decode(base);
    pte_fields_t with_top = pte_decode(base | PTE_RESV_MASK);
    int ok = fields_equal(plain, with_top);
    checks++;
    if (!ok) {
        mismatches++;
        if (mismatches < 10)
            printf("RESERVED-LEAK base=%llx\n", (unsigned long long)base);
    }
    check_pte(base | PTE_RESV_MASK);
}

int main(void) {
    int k;
    uint64_t i, v;

    /* (a) Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..53. */
    for (k = 0; k < 54; k++) {
        uint64_t vals[3] = { 1ULL << k, (1ULL << k) - 1ULL,
                             (1ULL << k) + 1ULL };
        for (i = 0; i < 3; i++)
            check_pte(vals[i]);
    }

    /* (b) Reserved-bits row: the 2^k directed values with bits 63:54
     * set, as a sanity check that the top bits are never read. */
    for (k = 0; k < 54; k++)
        check_reserved(1ULL << k);

    /* (c) Exhaustive 32-bit sweep: all pte in [0, 2^32). */
    for (v = 0; v < (1ULL << 32); v++)
        check_pte(v);

    /* (d) 10M fixed-seed splitmix64 raw 64-bit values. */
    sm_state = 0x123456789ABCDEF0ULL;
    for (i = 0; i < 10000000ULL; i++)
        check_pte(splitmix64());

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           (unsigned long long)checks, (unsigned long long)mismatches,
           (unsigned long long)hash);
    return mismatches == 0 ? 0 : 1;
}
