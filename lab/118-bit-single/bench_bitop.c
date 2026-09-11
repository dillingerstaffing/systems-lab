#define _POSIX_C_SOURCE 199309L /* clock_gettime under -std=c11 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bitop.h"

/* Throughput benchmark: the four single-bit operations over a
 * pre-generated deterministic input stream (1M (word, position) pairs from
 * fixed-seed splitmix64), timed with CLOCK_MONOTONIC, best of 5 runs.
 * Each loop iteration applies one operation (cycling set, clear, toggle,
 * test); results are checksummed into `sink` so the loop cannot be
 * optimized away. ns/value is per single-bit operation. */

#define NPRE 1048576ull
#define NOPS 50000000ull

static uint64_t rng_state = 0x123456789ABCDEF0ull;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint64_t pre_w[NPRE];
static int pre_p[NPRE];

int main(void)
{
    uint64_t best_ns = 0;
    uint64_t sink = 0;
    uint64_t n;
    int rep;

    /* Pre-generate all inputs before any timing starts. */
    for (n = 0; n < NPRE; n++) {
        pre_w[n] = splitmix64();
        pre_p[n] = (int)(splitmix64() % 64ull);
    }

    for (rep = 0; rep < 5; rep++) {
        struct timespec t0, t1;
        uint64_t ns;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (n = 0; n < NOPS; n++) {
            uint64_t x = pre_w[n % NPRE];
            int p = pre_p[n % NPRE];
            switch (n & 3u) {
            case 0: sink += bset(x, p); break;
            case 1: sink += bclr(x, p); break;
            case 2: sink += btg(x, p); break;
            default: sink += (uint64_t)btst(x, p); break;
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        ns = (uint64_t)(t1.tv_sec - t0.tv_sec) * 1000000000ull
           + (uint64_t)(t1.tv_nsec - t0.tv_nsec);
        if (best_ns == 0 || ns < best_ns)
            best_ns = ns;
        printf("rep %d: %llu ns total, %.3f ns/value, sink=0x%016llx\n",
               rep, (unsigned long long)ns,
               (double)ns / (double)NOPS, (unsigned long long)sink);
    }
    printf("best: %.3f ns/value over %llu operations\n",
           (double)best_ns / (double)NOPS, (unsigned long long)NOPS);
    return 0;
}
