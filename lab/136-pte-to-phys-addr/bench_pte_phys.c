#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "pte_phys.h"

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
    uint64_t *ppns = malloc(N * sizeof *ppns);
    uint64_t *offs = malloc(N * sizeof *offs);
    if (!ppns || !offs) { printf("malloc failed\n"); return 1; }
    for (uint64_t i = 0; i < N; i++) {
        ppns[i] = splitmix64(); /* full 64-bit PPN inputs */
        offs[i] = splitmix64(); /* full 64-bit offset inputs */
    }

    uint64_t best = UINT64_MAX;
    volatile unsigned int sink = 0;
    for (int r = 0; r < REPS; r++) {
        unsigned int acc = 0;
        uint64_t t0 = now_ns();
        for (uint64_t i = 0; i < N; i++)
            acc ^= (unsigned int)phys_addr_from_ppn(ppns[i], offs[i]) ^
                   (unsigned int)(phys_addr_from_ppn(ppns[i], offs[i]) >> 32);
        uint64_t t1 = now_ns();
        sink = acc; /* keep acc live */
        uint64_t dt = t1 - t0;
        printf("rep %d: %llu ns total, %.3f ns/conversion\n",
               r, (unsigned long long)dt, (double)dt / (double)N);
        if (dt < best) best = dt;
    }
    printf("best=%.3f ns/conversion\n", (double)best / (double)N);
    printf("sink=%u (accumulator kept live)\n", sink);
    free(ppns);
    free(offs);
    return 0;
}
