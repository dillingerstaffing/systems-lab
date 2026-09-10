/*
 * test_fmix64: differential verification of fmix64 (from fmix64.h)
 * against the verbatim published MurmurHash3 fmix64 (fmix64_ref.c),
 * and verification of the bijection invariant
 *     fmix64_inv(fmix64(x)) == x
 * on 10,000,000 fixed-seed splitmix64 64-bit values plus directed
 * edge cases.
 *
 * Usage: ./test_fmix64_o0 | ./test_fmix64_o2 | ./test_fmix64_asan
 * BUILD_NAME is set by the Makefile for each configuration.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fmix64.h"

uint64_t fmix64_reference(uint64_t k); /* fmix64_ref.c */

/* splitmix64, fixed seed: deterministic 64-bit stream */
static uint64_t splitmix_state;
static uint64_t splitmix_next(void)
{
    uint64_t z = (splitmix_state += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

/* FNV-1a 64 over a stream of words */
static uint64_t fnv_acc;
static void fnv_add(uint64_t w)
{
    fnv_acc ^= w;
    fnv_acc *= UINT64_C(0x100000001B3);
}

/* Recompute the modular inverse of an odd 64-bit value from scratch
 * with Newton-Raphson (inv <- inv*(2 - c*inv), six rounds) and check
 * it matches the constant baked into the header.  Independent of the
 * header's derivation: starts from inv = 1 with no shared code. */
static int check_inverse(const char *name, uint64_t c, uint64_t header_inv)
{
    uint64_t inv = 1;
    for (int i = 0; i < 6; i++)
        inv = inv * (2 - c * inv);
    if (inv != header_inv) {
        printf("  FAIL: %s Newton-Raphson inverse 0x%016llx != header 0x%016llx\n",
               name, (unsigned long long)inv, (unsigned long long)header_inv);
        return 1;
    }
    if (header_inv * c != 1) {
        printf("  FAIL: %s header_inv * c != 1 (got 0x%016llx)\n",
               name, (unsigned long long)(header_inv * c));
        return 1;
    }
    printf("  %s: header inverse 0x%016llx verified, inv * c == 1\n",
           name, (unsigned long long)header_inv);
    return 0;
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

static uint64_t total_cases;
static uint64_t total_mismatches;

int main(void)
{
    printf("fmix64 differential + bijection test, build %s\n", BUILD_NAME);

    /* [1/5] hand-checkable anchor: every step preserves 0 */
    printf("[1/5] anchor fmix64(0) == 0\n");
    {
        total_cases++;
        if (fmix64(0) != 0) {
            printf("  FAIL: fmix64(0) = 0x%016llx\n", (unsigned long long)fmix64(0));
            total_mismatches++;
        } else {
            printf("  fmix64(0) == 0, as each of the five steps fixes 0\n");
        }
    }

    /* [2/5] the hardcoded modular inverses, independently re-derived */
    printf("[2/5] multiply-inverse constants\n");
    {
        total_mismatches += (uint64_t)check_inverse("C1", FMIX64_C1, FMIX64_C1_INV);
        total_mismatches += (uint64_t)check_inverse("C2", FMIX64_C2, FMIX64_C2_INV);
    }

    /* [3/5] differential + bijection on 10M splitmix64 values */
    printf("[3/5] 10M fixed-seed splitmix64 64-bit values\n");
    {
        const uint64_t N = 10000000;
        uint64_t diff_mm = 0, bij_mm = 0;
        splitmix_state = UINT64_C(0x123456789ABCDEF0);
        fnv_acc = UINT64_C(0xCBF29CE484222325);
        for (uint64_t i = 0; i < N; i++) {
            uint64_t x = splitmix_next();
            uint64_t h = fmix64(x);
            if (h != fmix64_reference(x))
                diff_mm++;
            if (fmix64_inv(h) != x)
                bij_mm++;
            fnv_add(h);
        }
        total_cases += N;
        total_mismatches += diff_mm + bij_mm;
        printf("  differential mismatches: %llu\n", (unsigned long long)diff_mm);
        printf("  bijection mismatches:    %llu\n", (unsigned long long)bij_mm);
        printf("  done: cases=%llu\n", (unsigned long long)N);
    }

    /* [4/5] directed edge cases: differential and bijection on
     * structurally extreme words (empty, full, single bits on both
     * sides of the 33-bit shift boundary, alternating patterns,
     * half-word boundaries, multiply-constant neighborhoods). */
    printf("[4/5] directed edge cases\n");
    {
        static const uint64_t edge[] = {
            0, 1, 2, 3, 4, 63, 64, 65, 127, 128, 255, 256,
            UINT64_C(0x8000000000000000), UINT64_C(0x4000000000000000),
            UINT64_C(0x0000000080000000), UINT64_C(0x0000000040000000),
            UINT64_C(0x0000000100000000), UINT64_C(0x00000000FFFFFFFF),
            UINT64_C(0xFFFFFFFF00000000), UINT64_C(0x00000000FFFFFFFE),
            UINT64_C(0xFFFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFFFFFFFFFE),
            UINT64_C(0x5555555555555555), UINT64_C(0xAAAAAAAAAAAAAAAA),
            UINT64_C(0xFF51AFD7ED558CCD), UINT64_C(0xC4CEB9FE1A85EC53),
            UINT64_C(0xFF51AFD7ED558CCC), UINT64_C(0xC4CEB9FE1A85EC52),
            UINT64_C(0x0101010101010101), UINT64_C(0x8080808080808080),
            UINT64_C(0xDEADBEEFDEADBEEF), UINT64_C(0x123456789ABCDEF0),
            UINT64_C(0x0FEDCBA987654321),
        };
        uint64_t mm = 0;
        uint64_t n = sizeof(edge) / sizeof(edge[0]);
        for (uint64_t i = 0; i < n; i++) {
            uint64_t x = edge[i];
            uint64_t h = fmix64(x);
            if (h != fmix64_reference(x)) {
                printf("  DIFF FAIL x=0x%016llx\n", (unsigned long long)x);
                mm++;
            }
            if (fmix64_inv(h) != x) {
                printf("  BIJ FAIL x=0x%016llx\n", (unsigned long long)x);
                mm++;
            }
        }
        /* all 128 single-bit words, both as inputs and as hash images */
        for (int b = 0; b < 64; b++) {
            uint64_t x = UINT64_C(1) << b;
            if (fmix64_inv(fmix64(x)) != x) { printf("  BIJ FAIL x=1<<%d\n", b); mm++; }
            if (fmix64(fmix64_inv(x)) != x) { printf("  FWD FAIL h=1<<%d\n", b); mm++; }
        }
        total_cases += n + 128;
        total_mismatches += mm;
        printf("  done: cases=%llu mismatches=%llu\n",
               (unsigned long long)(n + 128), (unsigned long long)mm);
    }

    /* [5/5] throughput at this build's optimization level */
    printf("[5/5] throughput (PRNG pre-generated, excluded from timing)\n");
    {
        const uint64_t N = 4000000;
        uint64_t *in = malloc(N * sizeof(*in));
        if (!in) {
            printf("  allocation failed\n");
            return 2;
        }
        splitmix_state = UINT64_C(0x123456789ABCDEF0);
        for (uint64_t i = 0; i < N; i++)
            in[i] = splitmix_next();
        double best = 1e9;
        double passes[5];
        volatile uint64_t sink;
        for (int p = 0; p < 5; p++) {
            double t0 = now_s();
            uint64_t acc = 0;
            for (uint64_t i = 0; i < N; i++)
                acc ^= fmix64(in[i]);
            double t1 = now_s();
            sink = acc;
            passes[p] = (t1 - t0) / (double)N * 1e9;
            if (passes[p] < best)
                best = passes[p];
            printf("  throughput pass %d: %.3f ns/value (%.3f M values/s)\n",
                   p, passes[p], 1000.0 / passes[p]);
        }
        (void)sink;
        free(in);
        printf("  throughput best of 5: %.3f ns/value (%.3f M values/s)\n",
               best, 1000.0 / best);
    }

    printf("total verification cases: %llu\n", (unsigned long long)total_cases);
    printf("total mismatches: %llu\n", (unsigned long long)total_mismatches);
    printf("FNV-1a checksum of all outputs: 0x%016llx\n", (unsigned long long)fnv_acc);
    printf("RESULT: %s\n", total_mismatches == 0 ? "PASS" : "FAIL");
    return total_mismatches == 0 ? 0 : 1;
}
