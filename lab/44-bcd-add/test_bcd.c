#define _POSIX_C_SOURCE 200809L
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bcd.h"

/* FNV-1a 64-bit, used to fingerprint the full result stream. */
static uint64_t fnv1a(uint64_t h, uint64_t v)
{
    h ^= v;
    h *= 1099511628211ULL;
    return h;
}

/* Independent reference: unpack each nibble as a decimal digit (valid only
 * for valid packed BCD inputs, which is exactly the tested domain), do
 * ordinary integer arithmetic, repack into packed BCD. Shares no
 * correction logic with bcd_add / bcd_sub. */
static uint8_t ref_bcd_add(uint8_t a, uint8_t b, uint8_t *carry)
{
    unsigned da = 10u * (a >> 4) + (a & 15u);
    unsigned db = 10u * (b >> 4) + (b & 15u);
    unsigned s = da + db;
    *carry = (s >= 100u) ? 1 : 0;
    s %= 100u;
    return (uint8_t)(((s / 10u) << 4) | (s % 10u));
}

static uint8_t ref_bcd_sub(uint8_t a, uint8_t b, uint8_t *borrow)
{
    unsigned da = 10u * (a >> 4) + (a & 15u);
    unsigned db = 10u * (b >> 4) + (b & 15u);
    unsigned d;
    *borrow = (da < db) ? 1 : 0;
    d = *borrow ? da + 100u - db : da - db;
    return (uint8_t)(((d / 10u) << 4) | (d % 10u));
}

/* repack(n): pack the two decimal digits of n (0..99) into a BCD byte. */
static uint8_t repack(unsigned n)
{
    return (uint8_t)(((n / 10u) << 4) | (n % 10u));
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* TIMING_ITERS iterations, each doing one bcd_add and one bcd_sub over a
 * cycling 100x100 valid-pair table, results xor-folded into acc. */
#define TIMING_ITERS 20000000UL

int main(void)
{
    uint64_t h = 14695981039346656037ULL;
    unsigned long add_cases = 0, sub_cases = 0, mismatches = 0;
    unsigned long add_carries = 0, sub_borrows = 0;
    unsigned a, b;

    /* Exhaustive differential test over the entire defined domain:
     * all 100x100 valid packed BCD pairs for add and for sub. */
    for (a = 0; a < 100; a++) {
        for (b = 0; b < 100; b++) {
            uint8_t av = repack(a), bv = repack(b);
            uint8_t carry, ref_carry, r1, r1r;
            uint8_t borrow, ref_borrow, r2, r2r;

            r1 = bcd_add(av, bv, &carry);
            r1r = ref_bcd_add(av, bv, &ref_carry);
            add_cases++;
            if (r1 != r1r || carry != ref_carry)
                mismatches++;
            add_carries += carry;
            h = fnv1a(h, ((uint64_t)r1 << 8) | carry);

            r2 = bcd_sub(av, bv, &borrow);
            r2r = ref_bcd_sub(av, bv, &ref_borrow);
            sub_cases++;
            if (r2 != r2r || borrow != ref_borrow)
                mismatches++;
            sub_borrows += borrow;
            h = fnv1a(h, ((uint64_t)r2 << 8) | borrow);
        }
    }

    /* Throughput: measured at -O2 over TIMING_ITERS iterations; exact
     * loop contents are the body below. acc is printed so the calls
     * cannot be optimized away. */
    uint8_t tbl[10000];
    for (a = 0; a < 100; a++)
        for (b = 0; b < 100; b++)
            tbl[a * 100 + b] = repack(a);
    uint8_t tbl2[10000];
    for (a = 0; a < 100; a++)
        for (b = 0; b < 100; b++)
            tbl2[a * 100 + b] = repack(b);

    uint64_t acc = 0;
    uint8_t flag;
    double t0 = now_s();
    for (unsigned long i = 0; i < TIMING_ITERS; i++) {
        unsigned long k = i % 10000UL;
        acc += bcd_add(tbl[k], tbl2[k], &flag);
        acc += (uint64_t)flag << 56;
        acc += (uint64_t)bcd_sub(tbl[k], tbl2[k], &flag) << 32;
        acc += (uint64_t)flag << 48;
    }
    double dt = now_s() - t0;
    double ns_per_op = dt * 1e9 / (2.0 * (double)TIMING_ITERS);

    printf("add_cases=%lu\n", add_cases);
    printf("sub_cases=%lu\n", sub_cases);
    printf("mismatches=%lu\n", mismatches);
    printf("add_carry_out_ones=%lu\n", add_carries);
    printf("sub_borrow_out_ones=%lu\n", sub_borrows);
    printf("fnv1a=%016" PRIx64 "\n", h);
    printf("throughput_acc=%016" PRIx64 "\n", acc);
    printf("throughput_time_s=%.3f\n", dt);
    printf("throughput_ns_per_op=%.3f\n", ns_per_op);
    printf("run_exit=%d\n", mismatches == 0 ? 0 : 1);
    return mismatches == 0 ? 0 : 1;
}
