#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "pte_ad.h"

/*
 * Throughput bench: one ad_predict walk per iteration over a small
 * scripted tree. Each iteration targets a gigapage leaf whose A and D
 * bits are cleared again before the call, so every iteration exercises
 * the full walk plus the A/D update path. Best of 5 reps.
 */

#define N 2000000

int main(void)
{
    static pte_node_t nodes[3];
    pte_tree_t t = {nodes, 3};
    int n, i, rep, k;
    struct timespec t0, t1;
    double best = 1e30;

    memset(nodes, 0, sizeof(nodes));
    for (n = 0; n < 3; n++)
        for (i = 0; i < 512; i++)
            nodes[n].child[i] = -1;
    nodes[0].e[5] = 1ULL | (1ULL << 10); /* pointer, ppn 0 */
    nodes[0].child[5] = 1;
    nodes[0].e[7] = 1ULL | (1ULL << 1) | (1ULL << 2) | (1ULL << 4);
    /* gigapage leaf: V R W U, A=D=0 */
    nodes[1].e[3] = 1ULL | (1ULL << 10);
    nodes[1].child[3] = 2;
    nodes[2].e[11] = 1ULL | (1ULL << 1) | (1ULL << 2) | (1ULL << 4);

    for (rep = 0; rep < 5; rep++) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (k = 0; k < N; k++) {
            uint64_t va = ((uint64_t)7 << 30); /* vpn2=7: gigapage */
            ad_result_t r = ad_predict(&t, va, AD_ACCESS_STORE,
                                       AD_MODE_S);
            (void)r;
            nodes[0].e[7] &= ~((1ULL << 6) | (1ULL << 7)); /* clear A/D */
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        {
            double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                        (double)(t1.tv_nsec - t0.tv_nsec);
            ns /= N;
            if (ns < best)
                best = ns;
        }
    }
    printf("%.3f ns/predict at -O2 (best of 5, N=%d)\n", best, N);
    return 0;
}
