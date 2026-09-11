#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "pte_decode.h"

#define N 10000000UL
#define REPS 5

static uint64_t sm_state = 0x123456789ABCDEF0ULL;

static uint64_t splitmix64(void) {
    uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    uint64_t *vals = malloc(N * sizeof *vals);
    if (!vals) { printf("malloc failed\n"); return 1; }
    for (uint64_t i = 0; i < N; i++)
        vals[i] = splitmix64(); /* raw 64-bit PTE words */

    uint64_t best = UINT64_MAX;
    volatile unsigned int sink = 0;
    for (int r = 0; r < REPS; r++) {
        unsigned int acc = 0;
        uint64_t t0 = now_ns();
        for (uint64_t i = 0; i < N; i++) {
            pte_fields_t f = pte_decode(vals[i]);
            acc ^= (unsigned int)f.ppn ^ ((unsigned int)f.ppn >> 16) ^
                   (unsigned int)f.rsw ^
                   ((unsigned int)f.v | ((unsigned int)f.r << 1) |
                    ((unsigned int)f.w << 2) | ((unsigned int)f.x << 3) |
                    ((unsigned int)f.u << 4) | ((unsigned int)f.g << 5) |
                    ((unsigned int)f.a << 6) | ((unsigned int)f.d << 7));
        }
        uint64_t t1 = now_ns();
        sink = acc; /* keep acc live */
        uint64_t dt = t1 - t0;
        printf("rep %d: %llu ns total, %.3f ns/value\n",
               r, (unsigned long long)dt, (double)dt / (double)N);
        if (dt < best) best = dt;
    }
    printf("best=%.3f ns/value\n", (double)best / (double)N);
    printf("sink=%u (accumulator kept live)\n", sink);
    free(vals);
    return 0;
}
