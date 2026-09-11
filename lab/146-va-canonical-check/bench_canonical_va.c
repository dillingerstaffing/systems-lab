#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#include "canonical_va.h"

static uint64_t rng_state = 0x123456789ABCDEF0ULL;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void)
{
    const size_t N = 10000000;
    const int REPS = 5;
    uint64_t *buf = malloc(N * sizeof(*buf));
    uint64_t acc = 0;
    double best = 0.0;
    int r;
    size_t i;

    if (!buf) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    for (i = 0; i < N; i++)
        buf[i] = splitmix64();

    for (r = 0; r < REPS; r++) {
        double t0 = now_ns();
        for (i = 0; i < N; i++)
            acc ^= (uint64_t)(uint32_t)canonical_va(buf[i]);
        double t1 = now_ns();
        double dt = t1 - t0;
        if (r == 0 || dt < best)
            best = dt;
    }

    printf("acc: %" PRIu64 " (accumulate sink, prevents loop elimination)\n", acc);
    printf("best of %d reps, %zu values: %.3f ns/value\n", REPS, N, best / (double)N);
    printf("HEADER-LINE\n");
    printf("Throughput: %.3f ns/value at -O2\n", best / (double)N);
    free(buf);
    return 0;
}
