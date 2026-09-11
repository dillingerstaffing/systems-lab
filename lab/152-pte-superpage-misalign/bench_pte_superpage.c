#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "pte_superpage.h"

/* Throughput bench: best of 5 reps over N mixed (level, ppn) calls. */
#define N 50000000ULL
#define REPS 5

static uint64_t rng_state = 0x123456789ABCDEF0ULL;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static double now_s(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(void)
{
    double best = 1e30;
    int rep;

    for (rep = 0; rep < REPS; rep++) {
        unsigned long long n;
        volatile int sink = 0; /* defeat dead-call elimination */
        double t0 = now_s();

        for (n = 0; n < N; n++) {
            uint64_t ppn = rng_next() & 0xFFFFFFFFFFFULL;
            int level = (int)(n % 3);

            sink += pte_superpage_misaligned(level, ppn);
        }

        {
            double dt = now_s() - t0;
            double ns = dt / (double)N * 1e9;

            if (ns < best)
                best = ns;
        }
        (void)sink;
    }

    printf("%.3f ns/predict at -O2 (best of %d, N=%llu)\n",
           best, REPS, N);
    return 0;
}
