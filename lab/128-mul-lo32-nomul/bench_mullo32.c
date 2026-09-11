/*
 * Throughput bench for mullo32 (lab/128-mul-lo32-nomul), -O2.
 *
 * The timed loop per value does exactly three things: one LCG step to
 * produce the next 64-bit input, one call to mullo32 (compiled in a
 * separate translation unit, so the call is real; no LTO), and one
 * 64-bit accumulate into a sink that is printed, so the calls cannot be
 * eliminated as dead code. Reported ns/value is wall time divided by N
 * and therefore includes the LCG step and the accumulate alongside the
 * call. Best of 5 rounds.
 */
#define _POSIX_C_SOURCE 199309L /* clock_gettime, CLOCK_MONOTONIC */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "mul32.h"

#define N 20000000ull
#define ROUNDS 5

int main(void)
{
    double best = 1e30;

    for (int round = 0; round < ROUNDS; round++) {
        uint64_t x = 0x123456789ABCDEF0ull;
        uint64_t sink = 0;
        struct timespec t0, t1;

        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (uint64_t i = 0; i < N; i++) {
            x = x * 0x9E3779B97F4A7C15ull + 0xBF58476D1CE4E5B9ull;
            sink += mullo32(x, x ^ 0xA5A5A5A5A5A5A5A5ull);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);

        double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                    (double)(t1.tv_nsec - t0.tv_nsec);
        double per = ns / (double)N;
        printf("round %d: %.3f ns/value (sink=%016" PRIx64 ")\n",
               round, per, sink);
        if (per < best)
            best = per;
    }
    printf("best of %d: %.3f ns/value\n", ROUNDS, best);
    return 0;
}
