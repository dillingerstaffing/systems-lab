#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "msb_lsb.h"

#define DEBRUIJN64 UINT64_C(0x03f79d71b4cb0a89)

/* FNV-1a over the (ffs, fls) result stream; must match across builds. */
static uint64_t checksum = UINT64_C(14695981039346656037);

static void fold(int a, int b)
{
    checksum ^= (uint64_t)(uint32_t)(a + 0x100);
    checksum *= UINT64_C(1099511628211);
    checksum ^= (uint64_t)(uint32_t)(b + 0x100);
    checksum *= UINT64_C(1099511628211);
}

/*
 * Verify the de Bruijn constant's hash property directly: the 64
 * isolated single bits must hash to 64 distinct 6-bit values, and so
 * must the 64 all-ones prefixes 2^(k+1)-1. Without this, the tables in
 * msb_lsb.c could collide. Abort on failure.
 */
static void check_debruijn_property(void)
{
    int seen1[64] = { 0 };
    int seen2[64] = { 0 };
    unsigned k;

    for (k = 0u; k < 64u; k++) {
        uint64_t h1 = ((UINT64_C(1) << k) * DEBRUIJN64) >> 58;
        uint64_t h2 = ((~UINT64_C(0) >> (63u - k)) * DEBRUIJN64) >> 58;

        if (seen1[h1] || seen2[h2]) {
            printf("FAIL: de Bruijn hash collision at k=%u\n", k);
            exit(1);
        }
        seen1[h1] = 1;
        seen2[h2] = 1;
    }
}

static unsigned long long total_checks = 0;
static unsigned long long mismatches = 0;

static void check_value(uint64_t x)
{
    int got_ffs = ffs64(x);
    int want_ffs = __builtin_ffsll((long long)x);

    total_checks++;
    fold(got_ffs, x == 0u ? 0 : fls64(x));
    if (got_ffs != want_ffs) {
        printf("FFS MISMATCH: x=%" PRIu64 " got=%d want=%d\n", x, got_ffs,
               want_ffs);
        mismatches++;
    }
    if (x != 0u) {
        int got_fls = fls64(x);
        int want_fls = 63 - __builtin_clzll((unsigned long long)x);

        total_checks++;
        if (got_fls != want_fls) {
            printf("FLS MISMATCH: x=%" PRIu64 " got=%d want=%d\n", x,
                   got_fls, want_fls);
            mismatches++;
        }
    }
}

/* Fixed-seed xorshift64 for the timed loop. */
static uint64_t rng_state = UINT64_C(0x243F6A8885A308D3);

static uint64_t xorshift64(void)
{
    uint64_t x = rng_state;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    rng_state = x;
    return x;
}

/* Accumulator the optimizer cannot discard. */
static volatile long long sink;

static void measure_throughput(void)
{
    const unsigned long long N = 100000000ULL;
    unsigned long long i;
    struct timespec t0, t1;
    double ns_per_value;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < N; i++) {
        uint64_t v = xorshift64();
        sink += ffs64(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns_per_value = (double)((t1.tv_sec - t0.tv_sec) * 1000000000LL +
                            (t1.tv_nsec - t0.tv_nsec)) / (double)N;
    printf("throughput ffs64: %.2f ns/value over %llu timed values\n",
           ns_per_value, N);

    rng_state = UINT64_C(0x243F6A8885A308D3);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (i = 0; i < N; i++) {
        uint64_t v = xorshift64();
        sink += fls64(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ns_per_value = (double)((t1.tv_sec - t0.tv_sec) * 1000000000LL +
                            (t1.tv_nsec - t0.tv_nsec)) / (double)N;
    printf("throughput fls64: %.2f ns/value over %llu timed values\n",
           ns_per_value, N);
}

int main(void)
{
    uint64_t v;
    unsigned k;

    check_debruijn_property();

    /* Exhaustive: every 16-bit input. */
    for (v = 0u; v < 65536u; v++)
        check_value(v);

    /* Directed edges: 0, all-ones, every single bit, every 2^(k+1)-1. */
    check_value(0u);
    check_value(UINT64_C(0xFFFFFFFFFFFFFFFF));
    check_value(UINT64_C(0xAAAAAAAAAAAAAAAA));
    check_value(UINT64_C(0x5555555555555555));
    for (k = 0u; k < 64u; k++) {
        check_value(UINT64_C(1) << k);
        check_value(~UINT64_C(0) >> (63u - k));
    }
    /* fls64(0) is defined as -1 by the header contract. */
    if (fls64(0u) != -1) {
        printf("FLS MISMATCH: x=0 got=%d want=-1\n", fls64(0u));
        mismatches++;
    }
    total_checks++;

    printf("checks=%llu mismatches=%llu checksum=%" PRIu64 "\n", total_checks,
           mismatches, checksum);

    if (mismatches != 0u)
        return 1;

    measure_throughput();
    return 0;
}
