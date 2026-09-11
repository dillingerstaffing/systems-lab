#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>

#include "satp_mode_validate.h"

/* Throughput bench: best of 5 reps over N calls cycling MODE 0..15. */
#define N 50000000ULL
#define REPS 5

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

        for (n = 0; n < N; n++)
            sink += satp_mode_legal_rv64((unsigned)(n & 15));

        {
            double dt = now_s() - t0;
            double ns = dt / (double)N * 1e9;

            if (ns < best)
                best = ns;
        }
        (void)sink;
    }

    printf("%.3f ns/validate at -O2 (best of %d, N=%llu)\n",
           best, REPS, N);
    return 0;
}
