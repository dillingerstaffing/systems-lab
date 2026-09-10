/*
 * test_swar_sat_add.c - differential verification of swar_sat_add16x4.
 *
 * Usage: ./test_swar_sat_add [full|quick]   (default: full)
 *
 * Reference: independent per-lane scalar saturating add.  The SWAR word
 * result must equal the packed scalar results bit for bit.
 *
 * Coverage in "full" mode:
 *   - 10 hand-checked known-answer vectors (derivations in PROOF.md).
 *   - Directed crosstalk sweep: every 16-bit lane pair drawn from the
 *     boundary set {0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF},
 *     all 36^4 = 1,679,616 combinations across the four lanes, so every
 *     lane saturates next to every boundary value in its neighbors.
 *   - Exhaustive per-lane differential: all 2^32 pairs (x, y) of 16-bit
 *     values, each packed into all four lanes at once, so every lane
 *     position sees every possible input pair: 4,294,967,296 checks.
 *   - FNV-1a checksum over every result byte (directed + exhaustive).
 *
 * "quick" mode runs the known-answer vectors, the full directed
 * crosstalk sweep, and a fixed-seed 2^20 pseudo-random slice, and is
 * used for the -O0 and ASan+UBSan builds where the exhaustive loop is
 * too slow to be practical.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "swar_sat_add.h"

/* Independent reference: scalar per-lane saturating add. */
static uint64_t ref_sat_add(uint64_t a, uint64_t b)
{
    uint64_t r = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t x = (uint32_t)((a >> (16 * i)) & 0xFFFFU);
        uint32_t y = (uint32_t)((b >> (16 * i)) & 0xFFFFU);
        uint32_t s = x + y;
        if (s > 0xFFFFU)
            s = 0xFFFFU;
        r |= (uint64_t)s << (16 * i);
    }
    return r;
}

static uint64_t pack4(uint16_t l0, uint16_t l1, uint16_t l2, uint16_t l3)
{
    return (uint64_t)l0 | ((uint64_t)l1 << 16) |
           ((uint64_t)l2 << 32) | ((uint64_t)l3 << 48);
}

static uint64_t fnv1a;

static void checksum_word(uint64_t w)
{
    for (int i = 0; i < 8; i++) {
        fnv1a ^= (uint8_t)(w >> (8 * i));
        fnv1a *= 0x100000001B3ULL;
    }
}

static uint64_t mismatches;

static void check(uint64_t a, uint64_t b)
{
    uint64_t got = swar_sat_add16x4(a, b);
    uint64_t want = ref_sat_add(a, b);
    checksum_word(got);
    if (got != want) {
        if (mismatches < 8)
            printf("MISMATCH a=%016llx b=%016llx got=%016llx want=%016llx\n",
                   (unsigned long long)a, (unsigned long long)b,
                   (unsigned long long)got, (unsigned long long)want);
        mismatches++;
    }
}

struct kav {
    uint64_t a, b, want;
    const char *note;
};

int main(int argc, char **argv)
{
    int full = 1;
    if (argc > 1 && strcmp(argv[1], "quick") == 0)
        full = 0;

    fnv1a = 0xCBF29CE484222325ULL;
    mismatches = 0;

    /* Hand-checked known-answer vectors.  Derivations in PROOF.md. */
    static const struct kav vecs[] = {
        { 0x0000000000000000ULL, 0x0000000000000000ULL,
          0x0000000000000000ULL, "zero + zero" },
        { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL,
          0xFFFFFFFFFFFFFFFFULL, "all lanes saturate" },
        { 0x0001000200030004ULL, 0x0004000300020001ULL,
          0x0005000500050005ULL, "no lane overflows" },
        { 0xFFFF0000FFFF0000ULL, 0x0001FFFF0001FFFFULL,
          0xFFFFFFFFFFFFFFFFULL, "lanes 0,2,3 saturate; lane1 0x0000+0xFFFF" },
        { 0x0000FFFF00000000ULL, 0x0001000100010001ULL,
          0x0001FFFF00010001ULL, "lane2 saturates, neighbors untouched" },
        { 0x8000800080008000ULL, 0x8000800080008000ULL,
          0xFFFFFFFFFFFFFFFFULL, "0x8000+0x8000 saturates every lane" },
        { 0x8000800080008000ULL, 0x7FFF7FFF7FFF7FFFULL,
          0xFFFFFFFFFFFFFFFFULL, "0x8000+0x7FFF = 0xFFFF exact, no clamp" },
        { 0x000000000000FFFFULL, 0x0000000000000001ULL,
          0x000000000000FFFFULL, "lane0 0xFFFF+1 clamps, lane1 0+0" },
        { 0xFFFF000000000000ULL, 0x0001000000000000ULL,
          0xFFFF000000000000ULL, "top lane saturates, no carry lost" },
        { 0x00000000FFFF0000ULL, 0x0000000100010000ULL,
          0x00000001FFFF0000ULL, "lane1 saturates beside quiet lanes" },
    };
    uint64_t n_kav = 0;
    for (size_t i = 0; i < sizeof(vecs) / sizeof(vecs[0]); i++) {
        check(vecs[i].a, vecs[i].b);
        uint64_t got = swar_sat_add16x4(vecs[i].a, vecs[i].b);
        if (got != vecs[i].want) {
            printf("KAV FAIL [%s]: got %016llx want %016llx\n", vecs[i].note,
                   (unsigned long long)got, (unsigned long long)vecs[i].want);
            mismatches++;
        }
        n_kav++;
    }
    printf("known-answer vectors: %llu checked\n", (unsigned long long)n_kav);

    /* Directed crosstalk sweep: boundary lane values in every position. */
    static const uint16_t bounds[] = {
        0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFE, 0xFFFF
    };
    uint64_t n_cross = 0;
    for (int i0 = 0; i0 < 36; i0++)
        for (int i1 = 0; i1 < 36; i1++)
            for (int i2 = 0; i2 < 36; i2++)
                for (int i3 = 0; i3 < 36; i3++) {
                    int idx[4] = { i0, i1, i2, i3 };
                    uint16_t xa[4], xb[4];
                    for (int l = 0; l < 4; l++) {
                        xa[l] = bounds[idx[l] / 6];
                        xb[l] = bounds[idx[l] % 6];
                    }
                    check(pack4(xa[0], xa[1], xa[2], xa[3]),
                          pack4(xb[0], xb[1], xb[2], xb[3]));
                    n_cross++;
                }
    printf("directed crosstalk: %llu checks\n", (unsigned long long)n_cross);

    uint64_t n_exh = 0;
    if (full) {
        /* Exhaustive per-lane differential: every (x, y) in 2^32, packed
         * identically into all four lanes, so each lane position sees
         * every possible 16-bit input pair. */
        for (uint64_t x = 0; x < 0x10000ULL; x++) {
            uint64_t a = pack4((uint16_t)x, (uint16_t)x,
                               (uint16_t)x, (uint16_t)x);
            for (uint64_t y = 0; y < 0x10000ULL; y++) {
                uint64_t b = pack4((uint16_t)y, (uint16_t)y,
                                   (uint16_t)y, (uint16_t)y);
                check(a, b);
                n_exh++;
            }
        }
        printf("exhaustive lane pairs: %llu checks\n",
               (unsigned long long)n_exh);
    } else {
        /* Fixed-seed pseudo-random slice for the slow builds. */
        uint64_t s = 0x243F6A8885A308D3ULL;
        for (uint64_t i = 0; i < (1ULL << 20); i++) {
            s = s * 0x5851F42D4C957F2DULL + 0x14057B7EF767814FULL;
            uint64_t a = s;
            s = s * 0x5851F42D4C957F2DULL + 0x14057B7EF767814FULL;
            check(a, s);
            n_exh++;
        }
        printf("fixed-seed random slice: %llu checks\n",
               (unsigned long long)n_exh);
    }

    /* Throughput: timed loop of the SWAR op on pseudo-random inputs.
     * The loop includes word packing, the SWAR add, and a checksum
     * fold so the compiler cannot discard the work. */
    const uint64_t N = 100000000ULL;
    uint64_t s = 0x9E3779B97F4A7C15ULL, acc = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (uint64_t i = 0; i < N; i++) {
        s = s * 0x5851F42D4C957F2DULL + 0x14057B7EF767814FULL;
        uint64_t a = s;
        s = s * 0x5851F42D4C957F2DULL + 0x14057B7EF767814FULL;
        acc ^= swar_sat_add16x4(a, s);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double dt = (t1.tv_sec - t0.tv_sec) +
                (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("throughput: %.3f ns/op (%.1f Mops/s), N=%llu, acc=%016llx\n",
           dt / (double)N * 1e9, (double)N / dt / 1e6,
           (unsigned long long)N, (unsigned long long)acc);

    printf("fnv1a checksum: %016llx\n", (unsigned long long)fnv1a);
    printf("TOTAL mismatches: %llu\n", (unsigned long long)mismatches);
    return mismatches == 0 ? 0 : 1;
}
