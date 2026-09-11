#define _POSIX_C_SOURCE 199309L /* clock_gettime under -std=c11 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bitperm.h"

/* Throughput benchmark: perm64 (the 8-bit permutation applied to all eight
 * bytes of a 64-bit word) over a deterministic full-range 64-bit stream
 * (x += golden ratio each step, one add of loop overhead), timed with
 * CLOCK_MONOTONIC, best of 5 runs. Results are checksummed into `sink`
 * so the loop cannot be optimized away. */

int main(void)
{
    const uint64_t N = 20000000ull;
    uint64_t best_ns = 0;
    uint64_t sink = 0;
    int rep;

    for (rep = 0; rep < 5; rep++) {
        uint64_t x = 0x243F6A8885A308D3ull;
        struct timespec t0, t1;
        uint64_t ns, i;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (i = 0; i < N; i++) {
            x += 0x9E3779B97F4A7C15ull;
            sink += perm64(x);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        ns = (uint64_t)(t1.tv_sec - t0.tv_sec) * 1000000000ull
           + (uint64_t)(t1.tv_nsec - t0.tv_nsec);
        if (best_ns == 0 || ns < best_ns)
            best_ns = ns;
        printf("rep %d: %llu ns total, %.3f ns/value, sink=0x%016llx\n",
               rep, (unsigned long long)ns,
               (double)ns / (double)N, (unsigned long long)sink);
    }
    printf("best: %.3f ns/value over %llu values\n",
           (double)best_ns / (double)N, (unsigned long long)N);
    return 0;
}
