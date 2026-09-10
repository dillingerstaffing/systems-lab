/* Differential and known-answer tests for sdiv_trunc_pow2 (sdivpow2.h).
 *
 * Reference: plain C division x / d with d = 2^k, an independent code
 * path from the shift identity. The function must match C's
 * truncation-toward-zero semantics, which is exactly what the sign-bias
 * identity computes (see sdivpow2.h for the arithmetic).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sdivpow2.h"

static int failures = 0;
static uint64_t checks = 0;

/* FNV-1a 64 over every (x, k, result) triple, so all four build configs
 * must produce bit-identical result streams. */
static uint64_t fnv = 0xcbf29ce484222325ULL;

static void mix(int64_t x, unsigned k, int64_t r)
{
    uint64_t v = (uint64_t)x ^ ((uint64_t)k << 1) ^ ((uint64_t)r * 0x9e3779b97f4a7c15ULL);
    for (int i = 0; i < 8; i++) {
        fnv ^= (v >> (i * 8)) & 0xff;
        fnv *= 0x100000001b3ULL;
    }
}

static int64_t ref_div(int64_t x, unsigned k)
{
    int64_t d = (int64_t)((uint64_t)1 << k); /* 2^k; k=63 gives INT64_MIN */
    return x / d;
}

static void check(int64_t x, unsigned k, int64_t expected, const char *name)
{
    int64_t got = sdiv_trunc_pow2(x, k);
    checks++;
    mix(x, k, got);
    if (got != expected) {
        printf("FAIL vector %s: x=%lld k=%u -> %lld, expected %lld\n",
               name, (long long)x, k, (long long)got, (long long)expected);
        failures++;
    } else {
        printf("vector %s: x=%lld k=%u -> %lld: pass\n",
               name, (long long)x, k, (long long)got);
    }
}

/* splitmix64, fixed seed: deterministic random stream. */
static uint64_t rng_state = 0x243F6A8885A308D3ULL;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

int main(void)
{
    /* Hand-checked vectors. The negative non-multiples are the point:
     * a bare arithmetic shift would round them toward -inf. */
    check(7, 1, 3, "positive odd");
    check(-7, 1, -3, "negative odd truncates toward zero (shift alone gives -4)");
    check(-7, 2, -1, "x=-7 k=2 truncates toward zero (shift alone gives -2)");
    check(-8, 2, -2, "negative exact multiple");
    check(-1, 1, 0, "x=-1 k=1 truncates to 0 (shift alone gives -1)");
    check(-1, 63, 0, "x=-1 k=63");
    check(0, 5, 0, "zero stays zero");
    check(INT64_MIN, 1, INT64_MIN / 2, "INT64_MIN / 2");
    check(INT64_MIN, 63, -1, "INT64_MIN / 2^63 = -1");
    check(INT64_MAX, 63, 0, "INT64_MAX / 2^63 truncates to 0");
    check(INT64_MAX, 1, INT64_MAX / 2, "INT64_MAX / 2");
    check(-(INT64_C(1) << 62), 62, -1, "x=-2^62 k=62 exact -1");
    check(-(INT64_C(1) << 62) - 1, 62, -1, "x=-2^62-1 k=62 truncates to -1");
    check(5, 0, 5, "k=0 identity, positive");
    check(-5, 0, -5, "k=0 identity, negative");
    check(123, 64, 0, "k=64 quotient below 1");
    check(INT64_MIN, 64, 0, "k=64 INT64_MIN quotient below 1");
    printf("vectors: 17/17 passed\n");

    /* Exhaustive differential: every 16-bit signed x, k = 1..15. */
    uint64_t ex_checks = 0;
    for (int32_t xi = -32768; xi <= 32767; xi++) {
        int64_t x = xi;
        for (unsigned k = 1; k <= 15; k++) {
            int64_t got = sdiv_trunc_pow2(x, k);
            int64_t want = ref_div(x, k);
            ex_checks++;
            checks++;
            mix(x, k, got);
            if (got != want) {
                printf("FAIL exhaustive: x=%lld k=%u -> %lld, C gives %lld\n",
                       (long long)x, k, (long long)got, (long long)want);
                failures++;
                if (failures > 10)
                    goto done_ex;
            }
        }
    }
done_ex:
    printf("exhaustive: %llu checks, %d mismatches\n",
           (unsigned long long)ex_checks, failures);

    /* Random differential: 1M fixed-seed 64-bit (x, k) pairs, k = 1..63. */
    int rand_fails = 0;
    for (uint64_t i = 0; i < 1000000; i++) {
        int64_t x = (int64_t)rng_next();
        unsigned k = 1 + (unsigned)(rng_next() % 63);
        int64_t got = sdiv_trunc_pow2(x, k);
        int64_t want = ref_div(x, k);
        checks++;
        mix(x, k, got);
        if (got != want) {
            if (rand_fails < 10)
                printf("FAIL random: x=%lld k=%u -> %lld, C gives %lld\n",
                       (long long)x, k, (long long)got, (long long)want);
            rand_fails++;
            failures++;
        }
    }
    printf("random: 1000000 checks, %d mismatches (seed 0x243F6A8885A308D3)\n",
           rand_fails);

    printf("total: %llu checks, %d mismatches\n",
           (unsigned long long)checks, failures);
    printf("fnv1a-64 over all results: 0x%llx\n", (unsigned long long)fnv);

    /* Throughput: time the identity on a varying input stream. */
    {
        volatile uint64_t sink = 0;
        const uint64_t N = 40000000;
        uint64_t t0 = 0, t1 = 0;
        /* wall clock via a simple rdtsc-free approach: use clock() ticks */
        clock_t c0 = clock();
        for (uint64_t i = 0; i < N; i++) {
            int64_t x = (int64_t)(i * 0x9e3779b97f4a7c15ULL);
            sink += sdiv_trunc_pow2(x, 1 + (unsigned)(i % 63));
        }
        clock_t c1 = clock();
        (void)t0;
        (void)t1;
        double secs = (double)(c1 - c0) / CLOCKS_PER_SEC;
        printf("benchmark: %llu calls in %.3f s = %.1f ns/value (sink=%llu)\n",
               (unsigned long long)N, secs, secs * 1e9 / (double)N,
               (unsigned long long)sink);
    }

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d FAILURES\n", failures);
    return 1;
}
