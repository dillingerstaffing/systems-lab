#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "pte_step.h"

/* Throughput bench: classify N fixed-seed PTEs, best of 5 reps. */

static uint64_t rng_state = 0x9E3779B97F4A7C15ULL;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

#define N 2000000u

static uint64_t ptes[N];
static int levels[N];

static double now_s(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(void)
{
    unsigned i, rep;
    double best = 1e30;
    volatile unsigned sink = 0;

    for (i = 0; i < N; i++) {
        ptes[i] = splitmix64();
        levels[i] = (int)(splitmix64() % 3u);
    }

    for (rep = 0; rep < 5; rep++) {
        double t0 = now_s();

        for (i = 0; i < N; i++) {
            pte_step_result_t r = pte_step(ptes[i], levels[i]);

            sink += (unsigned)r.verdict;
        }

        {
            double dt = now_s() - t0;

            if (dt < best)
                best = dt;
        }
    }

    printf("%.3f ns/step at -O2 (best of 5, N=%u, sink=%u)\n",
           best * 1e9 / (double)N, N, sink);
    return 0;
}
