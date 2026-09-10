#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "add128.h"

/* ------------------------------------------------------------------ */
/* Independent oracle: exact arithmetic in unsigned __int128.          */
/* __int128 appears only in this file, never in add128.c / add128.h.   */
/* Each __int128 addition below is exact because its true sum stays    */
/* below 2^65, so nothing wraps inside the 128-bit type. (A single     */
/* 129-bit sum would not fit in __int128, hence two exact stages; the  */
/* oracle never uses the (sum < a) identity the implementation does.)  */
/* ------------------------------------------------------------------ */
typedef unsigned __int128 u128;

static void oracle(uint64_t a_hi, uint64_t a_lo,
                   uint64_t b_hi, uint64_t b_lo,
                   uint64_t cin,
                   uint64_t *sum_hi, uint64_t *sum_lo,
                   uint64_t *carry_out)
{
    u128 lo_t = (u128)a_lo + (u128)b_lo + (u128)cin;
    uint64_t c_lo = (uint64_t)(lo_t >> 64);
    u128 hi_t = (u128)a_hi + (u128)b_hi + (u128)c_lo;
    *sum_lo = (uint64_t)lo_t;
    *sum_hi = (uint64_t)hi_t;
    *carry_out = (uint64_t)(hi_t >> 64);
}

/* splitmix64 with explicit state: the deterministic PRNG for the
 * random phase. */
static uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a folded per 64-bit word (offset basis 14695981039346656037,
 * prime 1099511628211). */
static uint64_t fnv1a_word(uint64_t h, uint64_t v)
{
    h ^= v;
    h *= 0x100000001B3ULL;
    return h;
}

static uint64_t checksum = 0xCBF29CE484222325ULL;
static uint64_t total_cases = 0;
static uint64_t mismatches = 0;
static uint64_t printed = 0;

/* One case: implementation vs oracle. sum_hi, sum_lo, and carry_out are
 * all compared; on success the inputs and outputs feed the checksum. */
static void check_case(uint64_t a_hi, uint64_t a_lo,
                       uint64_t b_hi, uint64_t b_lo,
                       uint64_t cin)
{
    uint64_t i_hi, i_lo, i_c, o_hi, o_lo, o_c;

    add128(a_hi, a_lo, b_hi, b_lo, cin, &i_hi, &i_lo, &i_c);
    oracle(a_hi, a_lo, b_hi, b_lo, cin, &o_hi, &o_lo, &o_c);
    total_cases++;

    if (i_hi == o_hi && i_lo == o_lo && i_c == o_c) {
        checksum = fnv1a_word(checksum, a_hi);
        checksum = fnv1a_word(checksum, a_lo);
        checksum = fnv1a_word(checksum, b_hi);
        checksum = fnv1a_word(checksum, b_lo);
        checksum = fnv1a_word(checksum, cin);
        checksum = fnv1a_word(checksum, i_hi);
        checksum = fnv1a_word(checksum, i_lo);
        checksum = fnv1a_word(checksum, i_c);
    } else {
        mismatches++;
        if (printed < 10) {
            printed++;
            printf("MISMATCH a=%016" PRIx64 "%016" PRIx64
                   " b=%016" PRIx64 "%016" PRIx64 " cin=%" PRIu64
                   " impl=%016" PRIx64 "%016" PRIx64 " c=%" PRIu64
                   " oracle=%016" PRIx64 "%016" PRIx64 " c=%" PRIu64 "\n",
                   a_hi, a_lo, b_hi, b_lo, cin,
                   i_hi, i_lo, i_c, o_hi, o_lo, o_c);
        }
    }
}

/* Directed edge rows: every corner of the carry formula. The
 * b_lo == UINT64_MAX rows pin the wrap-through-all-ones edge where the
 * bare identity (sum < a) alone would report carry 0. */
static const struct {
    uint64_t a_hi, a_lo, b_hi, b_lo, cin;
} edges[] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1},
    {0, 0, 0, UINT64_MAX, 0},
    {0, 0, 0, UINT64_MAX, 1},          /* bare (sum<a): 0<0 false, true carry 1 */
    {0, UINT64_MAX, 0, UINT64_MAX, 0},
    {0, UINT64_MAX, 0, UINT64_MAX, 1},
    {0, UINT64_MAX, 0, 1, 0},
    {0, UINT64_MAX, 0, 0, 1},
    {0, 1, 0, UINT64_MAX, 1},          /* s1 wraps to 0 with c1=1, then +1 */
    {0, 5, 0, UINT64_MAX, 1},          /* c1=1 and lo_sum lands exactly on a_lo */
    {0, UINT64_C(0xFFFFFFFFFFFFFFFE), 0, 1, 1},   /* s1 all-ones, cin wraps it to 0 */
    {0, 1, 0, UINT64_C(0xFFFFFFFFFFFFFFFE), 1},
    {0, UINT64_C(0x8000000000000000), 0, UINT64_C(0x8000000000000000), 0},
    {0, UINT64_C(0x8000000000000000), 0, UINT64_C(0x7FFFFFFFFFFFFFFF), 1},
    {UINT64_MAX, 0, UINT64_MAX, 0, 0},
    {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX, 1}, /* 2^129 - 1 */
    {UINT64_MAX, UINT64_MAX, 0, UINT64_MAX, 0},  /* lo carry 1 wraps hi to 0 */
    {5, UINT64_MAX, 7, 1, 0},                   /* lo carry 1 into hi 5+7 */
    {0, UINT64_MAX, UINT64_MAX, 1, 0},          /* hi wrap detector: 0+MAX+1 */
    {UINT64_MAX, 0, 0, 0, 1},
    {0, 1, UINT64_MAX, 1, 1},
    {UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA),
     UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555), 0},
    {UINT64_C(0xAAAAAAAAAAAAAAAA), UINT64_C(0xAAAAAAAAAAAAAAAA),
     UINT64_C(0x5555555555555555), UINT64_C(0x5555555555555555), 1},
    {0, 0, 0, 1, 1},
};

static void edge_cases(void)
{
    for (size_t i = 0; i < sizeof edges / sizeof edges[0]; i++)
        check_case(edges[i].a_hi, edges[i].a_lo, edges[i].b_hi,
                   edges[i].b_lo, edges[i].cin);
}

/* Exhaustive: every 16-bit (a_lo, b_lo) pair, carry_in 0 and 1,
 * high words zero. 2^16 * 2^16 * 2 = 8,589,934,592 cases. */
static void exhaustive_16bit(void)
{
    for (uint32_t a = 0; a < 65536u; a++) {
        for (uint32_t b = 0; b < 65536u; b++) {
            check_case(0, a, 0, b, 0);
            check_case(0, a, 0, b, 1);
        }
    }
}

/* 1,000,000 random full-width cases. Fixed seed 20260910 (the run
 * date), documented here so the stream is reproducible. */
static void random_64bit(void)
{
    uint64_t state = 20260910ULL;
    for (uint64_t i = 0; i < 1000000u; i++) {
        uint64_t a_lo = splitmix64(&state);
        uint64_t b_lo = splitmix64(&state);
        uint64_t a_hi = splitmix64(&state);
        uint64_t b_hi = splitmix64(&state);
        uint64_t cin = splitmix64(&state) & 1u;
        check_case(a_hi, a_lo, b_hi, b_lo, cin);
    }
}

/* Throughput: N timed add128 calls in a dependency chain. The timed
 * region per iteration is one add128 call (separate translation unit,
 * no LTO, so a genuine call) plus the input derivation below; the
 * carry_in of each call is the previous call's carry_out, so the
 * compiler cannot skip, reorder, or constant-fold the chain. The final
 * state is folded into a printed checksum so the loop stays live. */
static void bench(void)
{
    const uint64_t N = 200000000ULL;
    uint64_t a_hi = 0x123456789ABCDEF0ULL, a_lo = 0x0FEDCBA987654321ULL;
    uint64_t b_hi = 0xAAAAAAAAAAAAAAAAULL, b_lo = 0x5555555555555555ULL;
    uint64_t sh = 0, sl = 0, co = 0;
    struct timespec t0, t1;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (uint64_t i = 0; i < N; i++) {
        add128(a_hi, a_lo, b_hi, b_lo, co, &sh, &sl, &co);
        a_lo = sl ^ (i * 0x9E3779B97F4A7C15ULL);
        a_hi = sh + i;
        b_lo += 0x9E3779B97F4A7C15ULL;
        b_hi ^= sl;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);

    double ns = (t1.tv_sec - t0.tv_sec) * 1e9
              + (t1.tv_nsec - t0.tv_nsec);
    uint64_t h = 0xCBF29CE484222325ULL;
    h = fnv1a_word(h, sh);
    h = fnv1a_word(h, sl);
    h = fnv1a_word(h, co);
    h = fnv1a_word(h, a_hi);
    h = fnv1a_word(h, a_lo);
    h = fnv1a_word(h, b_hi);
    h = fnv1a_word(h, b_lo);
    printf("bench: N=%" PRIu64 " elapsed=%.3f ms ns/value=%.3f"
           " checksum=0x%016" PRIx64 "\n",
           N, ns / 1e6, ns / (double)N, h);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "quick";

    if (strcmp(mode, "bench") == 0) {
        bench();
        return 0;
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    edge_cases();
    if (strcmp(mode, "full") == 0)
        exhaustive_16bit();
    random_64bit();

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double s = (t1.tv_sec - t0.tv_sec)
             + (t1.tv_nsec - t0.tv_nsec) / 1e9;
    printf("mode=%s cases=%" PRIu64 " mismatches=%" PRIu64
           " checksum=0x%016" PRIx64 " elapsed=%.1fs\n",
           mode, total_cases, mismatches, checksum, s);
    return mismatches ? 1 : 0;
}
