/* test_sat_sub.c - differential test of sat_sub64 against an exact
 * reference.
 *
 * Reference oracle: ref(a, b) = (a >= b) ? (uint64_t)((unsigned __int128)a - b)
 * : 0. The subtraction is computed in unsigned __int128, which is an
 * exact integer domain for 64-bit operands, so the oracle cannot wrap.
 * The __int128 appears ONLY in the oracle; the implementation in
 * sat_sub.c is free of it and branch-free.
 *
 * Phase 1 (only with "full" on the command line): exhaustive over all
 * 2^32 pairs of uint16_t operands.
 * Phase 2: directed edge pairs across the full uint64_t range, plus a
 * boundary band around each edge value.
 * Phase 3: 10,000,000 fixed-seed splitmix64 random 64-bit pairs.
 *
 * Phases 2 and 3 run identically on every build; an FNV-1a checksum
 * over their outputs must be identical across -O0, -O2, and
 * ASan+UBSan builds. Any pair where sat_sub64 differs from the oracle
 * is a mismatch; 0 mismatches are required.
 */
#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sat_sub.h"

/* Fixed seed 0x243F6A8885A308D3 (fractional digits of pi): every run is
 * reproducible. */
static uint64_t rng_state = 0x243F6A8885A308D3ULL;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t oracle(uint64_t a, uint64_t b)
{
    return (a >= b) ? (uint64_t)((unsigned __int128)a - b) : 0;
}

/* FNV-1a over the raw bytes of (a, b, result) in invocation order. */
static uint64_t fnv = 14695981039346656037ULL;

static void fnv_add(uint64_t x)
{
    unsigned char *p = (unsigned char *)&x;
    size_t i;
    for (i = 0; i < sizeof(x); i++) {
        fnv ^= p[i];
        fnv *= 1099511628211ULL;
    }
}

static uint64_t mismatches = 0;
static uint64_t total_pairs = 0;

static void check(uint64_t a, uint64_t b)
{
    uint64_t got = sat_sub64(a, b);
    uint64_t want = oracle(a, b);
    mismatches += (uint64_t)(got != want);
    total_pairs++;
    fnv_add(a);
    fnv_add(b);
    fnv_add(got);
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "bench") == 0) {
        /* Throughput: 100M timed pairs. Includes the splitmix64 PRNG
         * step per pair, so this is a lower bound on per-pair cost of
         * the saturating subtract itself. */
        uint64_t i, sink = 0;
        double t0 = now_s();
        for (i = 0; i < 100000000ULL; i++) {
            uint64_t a = rng_next();
            uint64_t b = rng_next();
            sink ^= sat_sub64(a, b) ^ a ^ b;
        }
        double dt = now_s() - t0;
        printf("bench: 100000000 pairs, %.3f s, %.3f ns/pair "
               "(includes PRNG step, sink=%" PRIu64 ")\n",
               dt, dt * 1e9 / 1e8, sink);
        return 0;
    }

    /* Phase 1: exhaustive 16-bit pairs. */
    if (argc > 1 && strcmp(argv[1], "full") == 0) {
        uint32_t a, b;
        uint64_t pairs = 0;
        double t0 = now_s();
        for (a = 0; a <= 0xFFFFu; a++) {
            for (b = 0; b <= 0xFFFFu; b++) {
                check((uint64_t)a, (uint64_t)b);
                pairs++;
            }
            if ((a & 0x1FFFu) == 0)
                printf("exhaustive: a=%5u of 65535, pairs=%" PRIu64
                       ", mismatches=%" PRIu64 "\n",
                       a, pairs, mismatches);
        }
        {
            double dt = now_s() - t0;
            printf("phase 1 (exhaustive 16-bit pairs): %" PRIu64
                   " pairs, %" PRIu64 " mismatches, %.1f s "
                   "(%.3f ns/pair)\n",
                   pairs, mismatches, dt, dt * 1e9 / (double)pairs);
        }
    }

    /* Phase 2: directed edges plus a boundary band around each edge. */
    {
        static const uint64_t edges[] = {
            0x0000000000000000ULL, 0x0000000000000001ULL,
            0x0000000000000002ULL, 0x0000000000000003ULL,
            0x00000000FFFFFFFFULL, 0x0000000100000000ULL,
            0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL,
            0x7FFFFFFFFFFFFFFFULL, 0x8000000000000000ULL,
            0xFFFFFFFFFFFFFFFEULL, 0xFFFFFFFFFFFFFFFFULL
        };
        size_t n = sizeof(edges) / sizeof(edges[0]);
        size_t i, j;
        uint64_t before = total_pairs;
        for (i = 0; i < n; i++) {
            for (j = 0; j < n; j++)
                check(edges[i], edges[j]);
            /* Boundary band: b around a, including the exact borrow
             * point b == a and the one-below/one-above cases, plus the
             * extremes 0 and UINT64_MAX. Unsigned wrap is intentional
             * and well-defined for a - 2 on small a. */
            check(edges[i], 0);
            check(edges[i], edges[i] - 2);
            check(edges[i], edges[i] - 1);
            check(edges[i], edges[i]);
            check(edges[i], edges[i] + 1);
            check(edges[i], edges[i] + 2);
            check(edges[i], UINT64_MAX);
        }
        printf("phase 2 (directed edges + boundary band): %" PRIu64
               " pairs, %" PRIu64 " mismatches\n",
               total_pairs - before, mismatches);
    }

    /* Phase 3: 10,000,000 fixed-seed splitmix64 random 64-bit pairs. */
    {
        uint64_t i;
        uint64_t before = total_pairs;
        for (i = 0; i < 10000000ULL; i++) {
            uint64_t a = rng_next();
            uint64_t b = rng_next();
            check(a, b);
        }
        printf("phase 3 (splitmix64, seed 0x243F6A8885A308D3): %" PRIu64
               " pairs, %" PRIu64 " mismatches\n",
               total_pairs - before, mismatches);
    }

    printf("total pairs: %" PRIu64 ", total mismatches: %" PRIu64
           ", FNV-1a: 0x%016" PRIx64 "\n",
           total_pairs, mismatches, fnv);
    return (mismatches == 0) ? 0 : 1;
}
