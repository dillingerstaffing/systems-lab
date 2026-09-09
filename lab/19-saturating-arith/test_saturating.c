#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "saturating.h"

/* Fixed-seed PRNG for the random sweep: xorshift64*, fully reproducible. */
static uint64_t rng_state = 0x9E3779B97F4A7C15ull;

static uint64_t next_rand(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

static int32_t rand_i32(void)
{
    return (int32_t)(next_rand() >> 32);
}

/* Trivially correct reference: 64-bit arithmetic cannot overflow for
 * 32-bit operands, so clamping the exact sum is definitionally right. */
static int32_t ref_add(int32_t a, int32_t b)
{
    int64_t s = (int64_t)a + (int64_t)b;
    if (s > (int64_t)INT32_MAX)
        return INT32_MAX;
    if (s < (int64_t)INT32_MIN)
        return INT32_MIN;
    return (int32_t)s;
}

static int32_t ref_sub(int32_t a, int32_t b)
{
    int64_t d = (int64_t)a - (int64_t)b;
    if (d > (int64_t)INT32_MAX)
        return INT32_MAX;
    if (d < (int64_t)INT32_MIN)
        return INT32_MIN;
    return (int32_t)d;
}

static uint64_t total_cases;
static uint64_t mismatches;
static uint64_t checksum;

static void check_add(int32_t a, int32_t b)
{
    int32_t got = sat_add32(a, b);
    int32_t want = ref_add(a, b);
    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("ADD MISMATCH a=%d b=%d got=%d want=%d\n", a, b, got, want);
    }
    total_cases++;
    checksum ^= (uint64_t)(uint32_t)got * 0x9E3779B97F4A7C15ull + total_cases;
}

static void check_sub(int32_t a, int32_t b)
{
    int32_t got = sat_sub32(a, b);
    int32_t want = ref_sub(a, b);
    if (got != want) {
        mismatches++;
        if (mismatches < 10)
            printf("SUB MISMATCH a=%d b=%d got=%d want=%d\n", a, b, got, want);
    }
    total_cases++;
    checksum ^= (uint64_t)(uint32_t)got * 0x9E3779B97F4A7C15ull + total_cases;
}

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void)
{
    /* Directed: cross product of boundary and neighborhood values. Every
     * combination of INT32_MIN, INT32_MAX, 0, +-1, +-2, +-3 and the
     * values adjacent to the saturation points, for both ops. */
    static const int32_t vals[] = {
        INT32_MIN, INT32_MIN + 1, INT32_MIN + 2,
        -3, -2, -1, 0, 1, 2, 3,
        INT32_MAX - 2, INT32_MAX - 1, INT32_MAX
    };
    size_t n = sizeof(vals) / sizeof(vals[0]);
    uint64_t directed = 0;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            check_add(vals[i], vals[j]);
            check_sub(vals[i], vals[j]);
            directed += 2;
        }
    }
    printf("directed=%llu\n", (unsigned long long)directed);

    /* Random sweep: 5,000,000 pairs per op, fixed seed, timed. The timing
     * loop reuses one accumulator so the optimizer cannot delete the
     * work; the checksum printed at the end covers every result. */
    const uint64_t RANDOM_PAIRS = 5000000ull;

    double t0 = now_ns();
    for (uint64_t i = 0; i < RANDOM_PAIRS; i++)
        check_add(rand_i32(), rand_i32());
    double t1 = now_ns();
    double add_ns = t1 - t0;

    for (uint64_t i = 0; i < RANDOM_PAIRS; i++)
        check_sub(rand_i32(), rand_i32());
    double t2 = now_ns();
    double sub_ns = t2 - t1;

    printf("total_cases=%llu mismatches=%llu checksum=%llu\n",
           (unsigned long long)total_cases,
           (unsigned long long)mismatches,
           (unsigned long long)checksum);
    printf("add: pairs=5000000 ns_total=%.0f ns_per_op=%.2f\n",
           add_ns, add_ns / (double)RANDOM_PAIRS);
    printf("sub: pairs=5000000 ns_total=%.0f ns_per_op=%.2f\n",
           sub_ns, sub_ns / (double)RANDOM_PAIRS);

    if (mismatches != 0) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
