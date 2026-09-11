#define _POSIX_C_SOURCE 199309L /* clock_gettime */

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "satp_fields.h"

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
 * Each field is built bit-by-bit from single-bit extractions of the
 * input word at the spec's bit positions (MODE at 60+b, ASID at 44+b,
 * PPN at b), accumulated with OR. No whole-field shift, no composite
 * field mask, no code shared with the implementation beyond the field
 * boundaries themselves, which come from the spec layout.
 */
static satp_fields_t oracle_decode(uint64_t satp) {
    satp_fields_t f;
    int b;

    f.mode = 0;
    f.asid = 0;
    f.ppn = 0;
    for (b = 0; b < 4; b++)
        f.mode = (uint8_t)(f.mode |
                           (uint8_t)(((satp >> (60 + b)) & 1ULL) << b));
    for (b = 0; b < 16; b++)
        f.asid = (uint16_t)(f.asid |
                            (uint16_t)(((satp >> (44 + b)) & 1ULL) << b));
    for (b = 0; b < 44; b++)
        f.ppn |= ((satp >> b) & 1ULL) << b;
    return f;
}

static int fields_equal(satp_fields_t x, satp_fields_t y) {
    return x.mode == y.mode && x.asid == y.asid && x.ppn == y.ppn;
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
 * re-encoding the decoded fields must restore the input word
 * exactly. The three fields cover all 64 bits with no gaps, so the
 * recombine check is a literal recombine(decode(satp)) == satp.
 */
static void check_satp(uint64_t satp) {
    satp_fields_t got = satp_decode(satp);
    satp_fields_t want = oracle_decode(satp);
    int ok = fields_equal(got, want) &&
             (satp_recombine(got) == satp);

    /* Checksum folds the decoded output: mode, asid (2 bytes, low
     * byte first), then ppn low byte first (6 bytes cover 44 bits). */
    {
        uint64_t ppn = got.ppn;
        int b;
        hash = fnv1a_step(hash, got.mode);
        hash = fnv1a_step(hash, (uint8_t)(got.asid & 0xFFu));
        hash = fnv1a_step(hash, (uint8_t)(got.asid >> 8));
        for (b = 0; b < 6; b++) {
            hash = fnv1a_step(hash, (uint8_t)(ppn & 0xFFULL));
            ppn >>= 8;
        }
    }

    checks++;
    if (!ok) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH satp=%llx\n", (unsigned long long)satp);
    }
}

int main(void) {
    int k;
    uint64_t i, v;
    struct timespec t0, t1;
    double elapsed_ns;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* (a) Directed edges: 2^k, 2^k - 1, 2^k + 1 for k = 0..63.
     * The k = 43/44 and k = 59/60 rows straddle the PPN/ASID and
     * ASID/MODE field boundaries. */
    for (k = 0; k < 64; k++) {
        uint64_t vals[3] = { 1ULL << k, (1ULL << k) - 1ULL,
                             (1ULL << k) + 1ULL };
        for (i = 0; i < 3; i++)
            check_satp(vals[i]);
    }

    /* (b) Exhaustive ASID: all 65,536 ASID values in bits 59:44,
     * MODE and PPN zero. */
    for (v = 0; v < (1ULL << 16); v++)
        check_satp(v << 44);

    /* (c) Exhaustive 24-bit PPN lanes: every value of a 24-bit window
     * at lane offsets 0, 10, 20 inside bits 43:0. The three lanes
     * overlap (0-23, 10-33, 20-43) and together cover the full
     * 44-bit PPN; MODE and ASID zero in this row. */
    {
        int off;
        for (off = 0; off <= 20; off += 10)
            for (v = 0; v < (1ULL << 24); v++)
                check_satp(v << off);
    }

    /* (d) 10M fixed-seed splitmix64 raw 64-bit values: all three
     * fields exercised together, all 16 MODE values included. */
    sm_state = 0x123456789ABCDEF0ULL;
    for (i = 0; i < 10000000ULL; i++)
        check_satp(splitmix64());

    clock_gettime(CLOCK_MONOTONIC, &t1);
    elapsed_ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                 (double)(t1.tv_nsec - t0.tv_nsec);

    printf("checks=%llu mismatches=%llu checksum=%016llx\n",
           (unsigned long long)checks, (unsigned long long)mismatches,
           (unsigned long long)hash);
    printf("elapsed=%.3f s (%.3f ns/check)\n",
           elapsed_ns / 1e9, elapsed_ns / (double)checks);
    return mismatches == 0 ? 0 : 1;
}
