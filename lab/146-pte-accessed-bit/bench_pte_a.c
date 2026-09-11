#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>

#include "pte_a.h"

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void)
{
    const size_t NCASES = 256 * 3;
    const size_t ROUNDS = 200000;
    const size_t N = NCASES * ROUNDS;
    const int REPS = 5;
    uint8_t *flags = malloc(NCASES * sizeof(*flags));
    int *accs = malloc(NCASES * sizeof(*accs));
    uint64_t acc = 0;
    double best = 0.0;
    int r;
    size_t i, k;

    if (!flags || !accs) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }

    /* The full 768-case table, run round-robin. */
    k = 0;
    for (unsigned f = 0; f < 256; f++)
        for (int a = PTE_ACC_READ; a <= PTE_ACC_EXEC; a++) {
            flags[k] = (uint8_t)f;
            accs[k] = a;
            k++;
        }

    for (r = 0; r < REPS; r++) {
        double t0 = now_ns();
        for (i = 0; i < N; i++) {
            pte_a_result q = pte_a_check(flags[i % NCASES], accs[i % NCASES]);
            acc ^= (uint64_t)(uint32_t)(q.fault * 2 + q.a_after);
        }
        double t1 = now_ns();
        double dt = t1 - t0;
        if (r == 0 || dt < best)
            best = dt;
    }

    printf("acc: %" PRIu64 " (accumulate sink, prevents loop elimination)\n", acc);
    printf("best of %d reps, %zu values: %.3f ns/value\n", REPS, N, best / (double)N);
    printf("HEADER-LINE\n");
    printf("Throughput: %.3f ns/value at -O2\n", best / (double)N);
    free(flags);
    free(accs);
    return 0;
}
