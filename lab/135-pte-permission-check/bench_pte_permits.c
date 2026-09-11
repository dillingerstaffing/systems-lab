#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "pte_permits.h"

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
        /* Cycle through all 96 input combinations so every rule row
         * is timed; the accumulator keeps the loop live. */
        for (uint64_t i = 0; i < N; i++) {
            unsigned bits = (unsigned)(i % 96u);
            unsigned rv_r = (bits >> 0) & 1u;
            unsigned rv_w = (bits >> 1) & 1u;
            unsigned rv_x = (bits >> 2) & 1u;
            unsigned rv_u = (bits >> 3) & 1u;
            pte_access_t a = (pte_access_t)((bits >> 4) % 3u);
            pte_mode_t m = (pte_mode_t)((bits >> 6) & 1u);
            acc = acc * 31u + (unsigned)pte_permits(rv_r, rv_w, rv_x,
                                                   rv_u, a, m) + (unsigned)i;
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
