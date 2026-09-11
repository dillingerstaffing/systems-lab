#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "pte_perm.h"

#define CASES 1536
#define ITERS 200000

/* 1536 inputs: flags 0..255, access 0..2, mode 0..1, enumerated by i. */
static int run_case(int i)
{
    unsigned flags = (unsigned)(i % 256);
    int access = (i / 256) % 3;
    int mode = (i / (256 * 3)) % 2;
    return pte_perm_ok((uint8_t)flags, (perm_access_t)access, (perm_mode_t)mode);
}

static double wall_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(void)
{
    long sink = 0;
    double best = 1e300;

    for (int rep = 0; rep < 5; rep++) {
        double t0 = wall_s();
        for (int it = 0; it < ITERS; it++) {
            for (int i = 0; i < CASES; i++) {
                sink += run_case(i);
            }
        }
        double t1 = wall_s();
        double ns = (t1 - t0) * 1e9 / (double)(ITERS * CASES);
        if (ns < best) {
            best = ns;
        }
        printf("rep %d: %.3f ns/value\n", rep, ns);
    }
    printf("THROUGHPUT: %.3f ns/value (best of 5)\n", best);
    printf("SINK: %ld\n", sink);
    return 0;
}
