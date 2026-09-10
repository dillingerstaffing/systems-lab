#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "sign.h"

/*
 * Reference: comparison-based sign. Comparisons on signed integers
 * are fully defined; the reference exists only to check the
 * branchless construction against.
 */
static int64_t ref_sign64(int64_t x)
{
    return (int64_t)(x > 0) - (int64_t)(x < 0);
}

/* splitmix64: deterministic stream of test inputs. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;
static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 1469598103934665603ULL;
static void fnv(uint64_t v)
{
    fnv1a ^= v;
    fnv1a *= 1099511628211ULL;
}

/* Feed a signed value into the FNV-1a checksum as its bit pattern. */
static void fnv_sign(int64_t s)
{
    fnv((uint64_t)s);
}

static long long mismatches = 0;

static void check(int64_t x)
{
    int64_t got = sign64(x);
    int64_t want = ref_sign64(x);
    if (got != want) {
        printf("MISMATCH x=%lld got=%lld want=%lld\n",
               (long long)x, (long long)got, (long long)want);
        mismatches++;
    }
    fnv_sign(got);
}

int main(void)
{
    long long checks = 0;

    /* 1. Directed rows: the boundary values, printed for the record. */
    static const int64_t directed[] = {
        INT64_MIN, -1, 0, 1, INT64_MAX
    };
    printf("directed rows:\n");
    for (size_t i = 0; i < sizeof(directed) / sizeof(directed[0]); i++) {
        int64_t x = directed[i];
        int64_t got = sign64(x);
        printf("  x=%20lld sign64=%2lld ref=%2lld %s\n",
               (long long)x, (long long)got, (long long)ref_sign64(x),
               got == ref_sign64(x) ? "ok" : "FAIL");
        check(x);
        checks++;
    }

    /* 2. Exhaustive 16-bit inputs as int64. */
    for (int i = -32768; i <= 32767; i++) {
        check((int64_t)i);
        checks++;
    }

    /* 3. One million fixed-seed splitmix64 64-bit values. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 1000000; i++) {
        int64_t x = (int64_t)splitmix64();
        check(x);
        checks++;
    }

    printf("checks=%lld mismatches=%lld fnv1a=%016llx\n",
           checks, mismatches, (unsigned long long)fnv1a);

#ifdef BENCH
    /*
     * Throughput: 1M precomputed values, 25 passes, best of 5, at
     * -O2. Inputs are generated once; the timed region measures only
     * sign64 plus the loop itself.
     */
    int64_t *vals = malloc(1000000 * sizeof(int64_t));
    if (!vals) {
        printf("BENCH malloc failed\n");
        return 1;
    }
    rng_state = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < 1000000; i++)
        vals[i] = (int64_t)splitmix64();

    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile int64_t sink = 0;
        for (int pass = 0; pass < 25; pass++)
            for (int i = 0; i < 1000000; i++)
                sink ^= sign64(vals[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    free(vals);
    printf("bench: %.2f ns/value (%.1f Mvalues/s over 25M timed values, best of 5)\n",
           best / 25000000.0, 25000000.0 / (best / 1e3));
#endif

    return mismatches == 0 ? 0 : 1;
}
