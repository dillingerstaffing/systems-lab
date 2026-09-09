#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "align.h"

/*
 * Differential test: each primitive is checked against an independent
 * division/loop-based reference over (a) every 2^k boundary and its
 * neighbors for k = 0..31, plus UINT32_MAX, crossed with every
 * power-of-two alignment 1..2^31, and (b) 1,048,576 random 32-bit
 * values from a fixed-seed PRNG. The program exits nonzero on any
 * mismatch and prints the total case count on success.
 */

#define N_RANDOM 1048576u

/* Fixed-seed PRNG so the run is reproducible bit for bit. */
static uint32_t rng_state = 0x12345678u;
static uint32_t next_rand(void)
{
    uint32_t s = rng_state;
    s ^= s << 13u;
    s ^= s >> 17u;
    s ^= s << 5u;
    rng_state = s;
    return s;
}

/* ---- References (division / loop based, computed in uint32_t) ---- */

static uint32_t ref_align_down(uint32_t p, uint32_t a)
{
    return (p / a) * a;
}

static uint32_t ref_align_up(uint32_t p, uint32_t a)
{
    return ((p + (a - 1u)) / a) * a;
}

static int ref_is_pow2(uint32_t x)
{
    int bits = 0;
    for (int i = 0; i < 32; i++) {
        if (x & (1u << i)) {
            bits++;
        }
    }
    return bits == 1;
}

static uint32_t ref_round_up_pow2(uint32_t x)
{
    uint32_t p;
    if (x <= 1u) {
        return 1u;
    }
    p = 1u;
    while (p < x) {
        if (p > UINT32_MAX / 2u) {
            return 0u; /* next shift would not fit */
        }
        p <<= 1u;
    }
    return p;
}

/* ---- Checkers ---- */

static unsigned long total_cases = 0;
static unsigned long total_mismatches = 0;

#define CHECK(cond, ...) do { \
    total_cases++; \
    if (!(cond)) { \
        total_mismatches++; \
        printf("MISMATCH: "); \
        printf(__VA_ARGS__); \
        printf("\n"); \
        if (total_mismatches > 10) { \
            printf("too many mismatches, aborting\n"); \
            exit(1); \
        } \
    } \
} while (0)

static uint32_t boundary_ps[100];
static size_t build_boundary_ps(void)
{
    size_t n = 0;
    for (int k = 0; k < 32; k++) {
        uint64_t v = (uint64_t)1 << k;
        boundary_ps[n++] = (uint32_t)(v - 1u);       /* 2^k - 1 (0 when k = 0) */
        boundary_ps[n++] = (uint32_t)v;             /* 2^k */
        boundary_ps[n++] = (uint32_t)(v + 1u);      /* 2^k + 1, fits for k < 32 */
    }
    boundary_ps[n++] = UINT32_MAX;
    return n;
}

static void explicit_edge_cases(void)
{
    /* a = 1: identity. */
    CHECK(align_up_u32(0u, 1u) == 0u, "align_up(0, 1)");
    CHECK(align_down_u32(0u, 1u) == 0u, "align_down(0, 1)");
    CHECK(align_up_u32(UINT32_MAX, 1u) == UINT32_MAX, "align_up(MAX, 1)");
    CHECK(align_down_u32(UINT32_MAX, 1u) == UINT32_MAX, "align_down(MAX, 1)");

    /* Small hand-checked values. */
    CHECK(align_up_u32(1u, 2u) == 2u, "align_up(1, 2)");
    CHECK(align_down_u32(1u, 2u) == 0u, "align_down(1, 2)");
    CHECK(align_up_u32(16u, 16u) == 16u, "align_up(16, 16)");
    CHECK(align_up_u32(17u, 16u) == 32u, "align_up(17, 16)");
    CHECK(align_down_u32(31u, 16u) == 16u, "align_down(31, 16)");

    /* is_pow2 edges. */
    CHECK(is_pow2_u32(0u) == 0, "is_pow2(0)");
    CHECK(is_pow2_u32(1u) == 1, "is_pow2(1)");
    CHECK(is_pow2_u32(3u) == 0, "is_pow2(3)");
    CHECK(is_pow2_u32(UINT32_C(0x80000000)) == 1, "is_pow2(2^31)");
    CHECK(is_pow2_u32(UINT32_MAX) == 0, "is_pow2(MAX)");

    /* round_up_pow2 edges, including the defined overflow return 0. */
    CHECK(round_up_pow2_u32(0u) == 1u, "rup2(0)");
    CHECK(round_up_pow2_u32(1u) == 1u, "rup2(1)");
    CHECK(round_up_pow2_u32(2u) == 2u, "rup2(2)");
    CHECK(round_up_pow2_u32(3u) == 4u, "rup2(3)");
    CHECK(round_up_pow2_u32(UINT32_C(0x7FFFFFFF)) == UINT32_C(0x80000000),
          "rup2(2^31 - 1)");
    CHECK(round_up_pow2_u32(UINT32_C(0x80000000)) == UINT32_C(0x80000000),
          "rup2(2^31)");
    CHECK(round_up_pow2_u32(UINT32_C(0x80000001)) == 0u, "rup2(2^31 + 1)");
    CHECK(round_up_pow2_u32(UINT32_MAX) == 0u, "rup2(MAX)");
}

int main(void)
{
    size_t nps = build_boundary_ps();
    uint32_t a;
    size_t i;

    explicit_edge_cases();

    /* align_up / align_down: boundary p x every power-of-two a, then random. */
    for (i = 0; i < nps; i++) {
        for (a = 1u; a != 0u; a <<= 1u) {
            uint32_t p = boundary_ps[i];
            CHECK(align_up_u32(p, a) == ref_align_up(p, a),
                  "align_up p=%u a=%u got=%u want=%u",
                  p, a, align_up_u32(p, a), ref_align_up(p, a));
            CHECK(align_down_u32(p, a) == ref_align_down(p, a),
                  "align_down p=%u a=%u got=%u want=%u",
                  p, a, align_down_u32(p, a), ref_align_down(p, a));
        }
    }
    for (uint32_t r = 0; r < N_RANDOM; r++) {
        uint32_t p = next_rand();
        a = 1u << (next_rand() & 31u);
        CHECK(align_up_u32(p, a) == ref_align_up(p, a),
              "align_up rand p=%u a=%u", p, a);
        CHECK(align_down_u32(p, a) == ref_align_down(p, a),
              "align_down rand p=%u a=%u", p, a);
    }

    /* is_pow2 / round_up_pow2: boundary x, then random. */
    for (i = 0; i < nps; i++) {
        uint32_t x = boundary_ps[i];
        CHECK(is_pow2_u32(x) == ref_is_pow2(x),
              "is_pow2 x=%u got=%d want=%d", x, is_pow2_u32(x), ref_is_pow2(x));
        CHECK(round_up_pow2_u32(x) == ref_round_up_pow2(x),
              "rup2 x=%u got=%u want=%u",
              x, round_up_pow2_u32(x), ref_round_up_pow2(x));
    }
    for (uint32_t r = 0; r < N_RANDOM; r++) {
        uint32_t x = next_rand();
        CHECK(is_pow2_u32(x) == ref_is_pow2(x), "is_pow2 rand x=%u", x);
        CHECK(round_up_pow2_u32(x) == ref_round_up_pow2(x), "rup2 rand x=%u", x);
    }

    printf("total_cases=%lu mismatches=%lu\n", total_cases, total_mismatches);
    if (total_mismatches != 0) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
