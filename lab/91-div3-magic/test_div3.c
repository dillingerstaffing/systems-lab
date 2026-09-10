#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "div3.h"

/*
 * Oracle ground truth, used ONLY in this test file, never in the
 * implementation. Native unsigned division x / 3 is exact by
 * definition of C unsigned integer division (truncation toward
 * zero, C11 6.5.5p6).
 */

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

static long long checks = 0;
static long long mismatches = 0;

static void check(uint32_t x)
{
    uint32_t q = div3_u32(x);
    uint32_t ref = x / 3; /* oracle: native division, test only */
    if (q != ref) {
        printf("MISMATCH x=%08x -> %08x (ref %08x)\n", x, q, ref);
        mismatches++;
    }
    fnv(q);
    checks++;
}

int main(void)
{
    /* 1. Directed edges: 0, the residue classes mod 3 at both
       ends of the range, and powers of two boundaries. */
    static const uint32_t edges[] = {
        0x00000000u, 0x00000001u, 0x00000002u, 0x00000003u,
        0x00000004u, 0x00000005u, 0x00000006u,
        0xFFFFFFFCu, 0xFFFFFFFDu, 0xFFFFFFFEu, 0xFFFFFFFFu,
        0xAAAAAAAAu, 0x55555555u,
        0x80000000u, 0x80000001u, 0x80000002u,
        0xBFFFFFFFu, 0xC0000000u, 0xC0000001u,
    };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        uint32_t q = div3_u32(edges[i]);
        printf("edge x=%08x -> q=%08x\n", edges[i], q);
        check(edges[i]);
    }

    /* 2. Exhaustive 16-bit inputs: every x in [0, 2^16). */
    for (uint32_t x = 0; x < 65536u; x++)
        check(x);

    /* 3. One million fixed-seed splitmix64 random 32-bit values. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 1000000; i++)
        check((uint32_t)splitmix64());

    printf("checks=%lld mismatches=%lld fnv1a=%016llx\n",
           checks, mismatches, (unsigned long long)fnv1a);

#ifdef BENCH
    /* Throughput at -O2: 100M values, best of 5. Inputs are
       pre-generated from splitmix64 into a 1M-entry array before
       the timed loop, so input generation is NOT in the measured
       time; the timed loop includes the array read. The compiler
       cannot fold the loop because the inputs vary. */
    #define BENCH_N (1 << 20)
    static uint32_t buf[BENCH_N];
    rng_state = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < BENCH_N; i++)
        buf[i] = (uint32_t)splitmix64();

    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint32_t sink = 0;
        for (int i = 0; i < 100000000; i++)
            sink ^= div3_u32(buf[i & (BENCH_N - 1)]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    printf("bench: %.3f ns/value (%.1f Mvalues/s over 100M timed values, best of 5)\n",
           best / 100000000.0, 100000000.0 / (best / 1e3));
#endif

    return mismatches == 0 ? 0 : 1;
}
