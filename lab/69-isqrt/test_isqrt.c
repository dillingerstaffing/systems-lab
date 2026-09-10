/*
 * test_isqrt.c: verification for lab/69-isqrt.
 *
 * Modes:
 *   ./test_isqrt quick  - edge cases, differential test vs a naive binary
 *                         search reference (all 2^16 inputs + 1M fixed-seed
 *                         splitmix64 64-bit values), throughput measurement.
 *   ./test_isqrt full   - quick plus the exhaustive invariant check over
 *                         all 2^32 32-bit inputs, with an FNV-1a checksum of
 *                         every result so builds can be compared bit for bit.
 *
 * The invariant checked exhaustively: for each x, r = isqrt64(x) satisfies
 * r*r <= x < (r+1)*(r+1), computed in 64-bit arithmetic. For 32-bit inputs
 * r < 2^16, so both products fit in 64 bits with room to spare.
 *
 * The naive reference uses only division (m <= x/m), so no product ever
 * overflows and no float or libm is involved anywhere.
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime under -std=c11 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "isqrt.h"

static uint64_t failures = 0;

#define CHECK(cond, ...)                                            \
    do {                                                            \
        if (!(cond)) {                                              \
            failures++;                                             \
            printf("FAIL: ");                                       \
            printf(__VA_ARGS__);                                    \
            printf("\n");                                           \
        }                                                           \
    } while (0)

/* Naive reference: binary search for the largest m with m*m <= x.
 * The test m <= x/m is exact and overflow-free for m >= 1; m is never 0
 * here because lo < hi implies mid >= 1. */
static uint64_t isqrt_naive(uint64_t x)
{
    uint64_t lo = 0, hi = UINT64_C(0xFFFFFFFF); /* answer always < 2^32 */
    while (lo < hi) {
        uint64_t mid = lo + (hi - lo + 1) / 2;
        if (mid <= x / mid)
            lo = mid;
        else
            hi = mid - 1;
    }
    return lo;
}

static uint64_t splitmix64(uint64_t *s)
{
    uint64_t z = (*s += UINT64_C(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

static uint64_t fnv1a64(uint64_t h, uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        h ^= (uint8_t)(v >> (8 * i));
        h *= UINT64_C(0x100000001B3);
    }
    return h;
}

static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void test_edges(void)
{
    static const uint64_t edges[] = {
        0, 1, 2, 3, 4, 5, 8, 9, 15, 16, 17, 24, 25, 26,
        99, 100, 101,
        UINT64_C(0xFFFFFFFF),          /* 2^32 - 1 */
        UINT64_C(0x100000000),         /* 2^32 */
        UINT64_C(0x100000001),         /* 2^32 + 1 */
        UINT64_C(0x1FFFFFFFF),         /* 2^33 - 1 */
        UINT64_C(0x7FFFFFFFFFFFFFFF),  /* 2^63 - 1 */
        UINT64_C(0x8000000000000000),  /* 2^63 */
        UINT64_C(0xFFFFFFFFFFFFFFFE),  /* 2^64 - 2 */
        UINT64_C(0xFFFFFFFFFFFFFFFF),  /* 2^64 - 1 */
        UINT64_C(18446744065119617025),/* (2^32-1)^2, largest square < 2^64 */
        UINT64_C(18446744065119617024),/* (2^32-1)^2 - 1 */
        15241578750190521ULL,          /* 123456789^2 */
        15241578750190520ULL,
        15241578750190522ULL,
    };
    size_t n = sizeof(edges) / sizeof(edges[0]);
    for (size_t i = 0; i < n; i++) {
        uint64_t x = edges[i];
        uint64_t r = isqrt64(x);
        uint64_t want = isqrt_naive(x);
        CHECK(r == want, "edge x=%llu: got %llu want %llu",
              (unsigned long long)x,
              (unsigned long long)r, (unsigned long long)want);
    }
    printf("edges: %zu cases, %s\n", n, failures ? "FAILURES" : "ok");
}

/* Differential test: all 2^16 inputs, then 1M fixed-seed 64-bit values.
 * For 64-bit samples the invariant is checked as r*r <= x (r < 2^32, so
 * r*r fits) and r+1 > x/(r+1), which is equivalent to (r+1)^2 > x with no
 * overflow since r+1 <= 2^32. */
static void test_differential(void)
{
    uint64_t mism = 0;
    uint64_t checked = 0;

    for (uint64_t x = 0; x < (UINT64_C(1) << 16); x++) {
        if (isqrt64(x) != isqrt_naive(x))
            mism++;
        checked++;
    }

    uint64_t s = UINT64_C(0x123456789ABCDEF0);
    for (uint64_t i = 0; i < 1000000; i++) {
        uint64_t x = splitmix64(&s);
        uint64_t r = isqrt64(x);
        uint64_t want = isqrt_naive(x);
        if (r != want)
            mism++;
        /* invariant, overflow-free */
        uint64_t r1 = r + 1;
        if (!(r * r <= x && r1 > x / r1))
            mism++;
        checked++;
    }

    printf("differential: %llu values, %llu mismatches\n",
           (unsigned long long)checked, (unsigned long long)mism);
    if (mism)
        failures += mism;
}

/* Exhaustive invariant over all 2^32 32-bit inputs. r < 2^16 here, so
 * (r+1)*(r+1) fits in 64 bits. FNV-1a over every result, printed so the
 * -O0, -O2, and ASan+UBSan builds can be compared bit for bit. */
static void test_exhaustive(void)
{
    uint64_t bad = 0;
    uint64_t h = UINT64_C(0xCBF29CE484222325);
    double t0 = now_sec();
    for (uint64_t x = 0; x < (UINT64_C(1) << 32); x++) {
        uint64_t r = isqrt64(x);
        uint64_t r1 = r + 1;
        if (!(r * r <= x && x < r1 * r1))
            bad++;
        h = fnv1a64(h, r);
        if ((x & ((UINT64_C(1) << 28) - 1)) == 0)
            fprintf(stderr, "  ... %llu / 4294967296\r",
                    (unsigned long long)x);
    }
    double dt = now_sec() - t0;
    fprintf(stderr, "\n");
    printf("exhaustive 2^32: 4294967296 inputs, %llu invariant violations, "
           "fnv1a=%016llx, %.1f s\n",
           (unsigned long long)bad, (unsigned long long)h, dt);
    if (bad)
        failures += bad;
}

/* Throughput at the build's optimization level. The input array is filled
 * before timing starts, so the loop under test contains no PRNG step;
 * the reported ns/value is the isqrt64 cost only. A checksum over the
 * results proves the loop was not optimized away. */
static void test_throughput(void)
{
    enum { N = 1 << 20 };
    static uint64_t xs[N];
    uint64_t s = UINT64_C(0x123456789ABCDEF0);
    for (int i = 0; i < N; i++)
        xs[i] = splitmix64(&s);

    volatile uint64_t sink = 0;
    double t0 = now_sec();
    for (int rep = 0; rep < 20; rep++)
        for (int i = 0; i < N; i++)
            sink += isqrt64(xs[i]);
    double dt = now_sec() - t0;
    double ns = dt * 1e9 / (20.0 * N);
    printf("throughput: %.2f ns/value over %d values x 20 reps "
           "(no PRNG in timed loop, checksum %016llx)\n",
           ns, N, (unsigned long long)fnv1a64(UINT64_C(0xCBF29CE484222325), sink));
}

int main(int argc, char **argv)
{
    int full = argc > 1 && strcmp(argv[1], "full") == 0;
    printf("isqrt64 verification (%s)\n", full ? "full" : "quick");
    test_edges();
    test_differential();
    test_throughput();
    if (full)
        test_exhaustive();
    if (failures) {
        printf("RESULT: FAIL (%llu failures)\n", (unsigned long long)failures);
        return 1;
    }
    printf("RESULT: PASS\n");
    return 0;
}
