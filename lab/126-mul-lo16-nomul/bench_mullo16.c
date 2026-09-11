#define _POSIX_C_SOURCE 199309L

/*
 * lab/126: bench for mullo16 at -O2.
 *
 * Fixed seed splitmix64 feeds fresh operand pairs each iteration. Best of
 * 5 runs over 2^22 pairs per run. The timed loop honestly includes: two
 * splitmix64 draws, the mullo16 call, and accumulation into a local that is
 * printed at the end (so the calls cannot be optimized away).
 */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mul16.h"

static uint64_t splitmix64(uint64_t *s)
{
    uint64_t z = (*s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(void)
{
    const uint64_t N = 1u << 22; /* operand pairs per run */
    double best = 1e300;
    uint64_t sink = 0;

    for (int run = 0; run < 5; run++) {
        uint64_t rng = 0x123456789abcdefULL; /* fixed seed, reset each run */
        uint64_t acc = 0;
        double t0 = now_s();
        for (uint64_t i = 0; i < N; i++) {
            uint16_t a = (uint16_t)splitmix64(&rng);
            uint16_t b = (uint16_t)splitmix64(&rng);
            acc += mullo16(a, b);
        }
        double t1 = now_s();
        double ns = (t1 - t0) * 1e9 / (double)N;
        if (ns < best)
            best = ns;
        sink += acc;
        printf("run %d: %.3f ns/value\n", run, ns);
    }

    printf("best-of-5: %.3f ns/value\n", best);
    printf("sink=%llu\n", (unsigned long long)sink);
    return 0;
}
