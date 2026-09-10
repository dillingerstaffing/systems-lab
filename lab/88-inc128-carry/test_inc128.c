#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "inc128.h"

/*
 * Oracle ground truth, used ONLY in this test file, never in the
 * implementation. Unsigned __int128 arithmetic wraps modulo 2^128,
 * so ref = x + 1 is (x + 1) mod 2^128, and the final carry is 1
 * exactly when x was 2^128 - 1 (then ref wraps to 0).
 */
typedef unsigned __int128 u128ref;

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

static void check(uint64_t hi, uint64_t lo)
{
    inc128_res r = inc128((u128){ hi, lo });
    u128ref x = ((u128ref)hi << 64) | (u128ref)lo;
    u128ref ref = x + 1;
    int carry_ref = (x == (u128ref)-1);
    if (r.hi != (uint64_t)(ref >> 64) ||
        r.lo != (uint64_t)ref ||
        r.carry != carry_ref) {
        printf("MISMATCH hi=%016llx lo=%016llx"
               " -> hi=%016llx lo=%016llx carry=%d"
               " (ref hi=%016llx lo=%016llx carry=%d)\n",
               (unsigned long long)hi, (unsigned long long)lo,
               (unsigned long long)r.hi, (unsigned long long)r.lo, r.carry,
               (unsigned long long)(ref >> 64),
               (unsigned long long)ref, carry_ref);
        mismatches++;
    }
    fnv(r.hi);
    fnv(r.lo);
    fnv((uint64_t)r.carry);
    checks++;
}

/* Exhaustive sweep bounds (2^16 x 2^16 = 2^32 cases by default). */
#ifndef SWEEP_HI
#define SWEEP_HI 65536u
#endif
#ifndef SWEEP_LO
#define SWEEP_LO 65536u
#endif

int main(void)
{
    /* 1. Directed edge rows: the carry cascade lives or dies here. */
    static const uint64_t edges[][2] = {
        { 0x0000000000000000ULL, 0x0000000000000000ULL },
        { 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL },
        { 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL },
        { 0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL },
        { 0x0000000000000001ULL, 0xFFFFFFFFFFFFFFFFULL },
    };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        inc128_res r = inc128((u128){ edges[i][0], edges[i][1] });
        printf("edge hi=%016llx lo=%016llx -> hi=%016llx lo=%016llx carry=%d\n",
               (unsigned long long)edges[i][0], (unsigned long long)edges[i][1],
               (unsigned long long)r.hi, (unsigned long long)r.lo, r.carry);
        check(edges[i][0], edges[i][1]);
    }

    /* 2. Exhaustive 16-bit pairs: every (hi, lo) in [0, 2^16)^2. */
    for (uint32_t hi = 0; hi < SWEEP_HI; hi++)
        for (uint32_t lo = 0; lo < SWEEP_LO; lo++)
            check((uint64_t)hi, (uint64_t)lo);

    /* 3. One million fixed-seed splitmix64 random 64-bit pairs. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 1000000; i++) {
        uint64_t hi = splitmix64();
        uint64_t lo = splitmix64();
        check(hi, lo);
    }

    printf("checks=%lld mismatches=%lld fnv1a=%016llx\n",
           checks, mismatches, (unsigned long long)fnv1a);

#ifdef BENCH
    /* Throughput at -O2: 100M values, best of 5. Inputs come from
       splitmix64 inside the timed loop, so the measured time
       includes input generation; the compiler cannot fold the
       loop because splitmix64's state is carried. */
    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        rng_state = 0x9E3779B97F4A7C15ULL + (uint64_t)rep;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint64_t sink = 0;
        for (int i = 0; i < 100000000; i++) {
            inc128_res r = inc128((u128){ splitmix64(), splitmix64() });
            sink ^= r.hi ^ r.lo ^ (uint64_t)r.carry;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    printf("bench: %.2f ns/value (%.1f Mvalues/s over 100M timed values, best of 5)\n",
           best / 100000000.0, 100000000.0 / (best / 1e3));
#endif

    return mismatches == 0 ? 0 : 1;
}
