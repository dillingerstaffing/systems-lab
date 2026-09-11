#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "walk_permits.h"

#define N 10000000UL
#define REPS 5

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    uint64_t best = UINT64_MAX;
    volatile unsigned int sink = 0;

    for (int rep = 0; rep < REPS; rep++) {
        unsigned int acc = 0;
        uint64_t t0 = now_ns();
        /* Cycle through the 3-level exhaustive space (4096 level
         * bit-combos x 3 access x 2 mode = 24576 inputs) so every
         * walk shape is timed; the accumulator keeps the loop
         * live. */
        for (uint64_t i = 0; i < N; i++) {
            pte_bits_t L[3];
            uint64_t v = i % 24576ULL;
            walk_access_t a = (walk_access_t)(v % 3u);
            walk_mode_t m = (walk_mode_t)((v / 3u) % 2u);
            uint64_t bits = v / 6u;
            L[0].r = (unsigned)((bits >> 0) & 1u);
            L[0].w = (unsigned)((bits >> 1) & 1u);
            L[0].x = (unsigned)((bits >> 2) & 1u);
            L[0].u = (unsigned)((bits >> 3) & 1u);
            L[1].r = (unsigned)((bits >> 4) & 1u);
            L[1].w = (unsigned)((bits >> 5) & 1u);
            L[1].x = (unsigned)((bits >> 6) & 1u);
            L[1].u = (unsigned)((bits >> 7) & 1u);
            L[2].r = (unsigned)((bits >> 8) & 1u);
            L[2].w = (unsigned)((bits >> 9) & 1u);
            L[2].x = (unsigned)((bits >> 10) & 1u);
            L[2].u = (unsigned)((bits >> 11) & 1u);
            acc = acc * 31u + (unsigned)walk_permits(L, 3, a, m) +
                  (unsigned)i;
        }
        uint64_t t1 = now_ns();
        sink = acc; /* keep acc live */
        uint64_t dt = t1 - t0;
        printf("rep %d: %llu ns total, %.3f ns/check\n",
               rep, (unsigned long long)dt, (double)dt / (double)N);
        if (dt < best) best = dt;
    }
    printf("best=%.3f ns/check\n", (double)best / (double)N);
    printf("header-throughput=%.3f ns/check\n", (double)best / (double)N);
    printf("sink=%u (accumulator kept live)\n", sink);
    return 0;
}
