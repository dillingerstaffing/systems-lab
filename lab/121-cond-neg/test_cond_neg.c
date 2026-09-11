#define _POSIX_C_SOURCE 200809L
/*
 * Differential test for cond_neg (lab/121-cond-neg).
 *
 * The oracle is the plain if/else reference: f ? (0 - x) : x. That is a
 * structurally different computation from the implementation's
 * (x ^ -f) + f bit-twiddle, so agreement pins the identity rather than
 * a shared bug.
 *
 * With -DBENCH the correctness phases are skipped and throughput is
 * measured instead.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef BENCH
#include "cond_neg.c"
#else
#include "cond_neg.h"
#endif

/* Fixed-seed splitmix64, seeded with 0x123456789ABCDEF0 in main. */
static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

#ifdef BENCH
int main(void)
{
    /* Throughput: 25M values at -O2, best of 5. */
    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        rng_state = 0x123456789ABCDEF0ULL + (uint64_t)rep;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint64_t sink = 0;
        for (int i = 0; i < 25000000; i++)
            sink ^= cond_neg(rng_next(), rng_next() & 1ULL);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (double)(t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    printf("bench: %.2f ns/value (%.1f Mvalues/s over 25M timed values, "
           "best of 5)\n",
           best / 25000000.0, 25000000.0 / (best / 1e3));
    return 0;
}
#else

/* Oracle: if/else reference; the implementation is allowed no such
 * control flow, the oracle is. */
static uint64_t oracle_neg(uint64_t x, uint64_t f)
{
    if (f != 0)
        return 0ULL - x;
    return x;
}

/* FNV-1a over all results; must match across -O0, -O2, and sanitizers. */
static uint64_t fnv = 0xCBF29CE484222325ULL;

static void fnv_fold(uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        fnv ^= (v >> (8 * i)) & 0xFFu;
        fnv *= 0x100000001B3ULL;
    }
}

static uint64_t mismatches = 0;
static uint64_t total_checks = 0;

static void check_pair(uint64_t x, uint64_t f)
{
    uint64_t got = cond_neg(x, f);
    uint64_t want = oracle_neg(x, f);
    mismatches += (uint64_t)(got != want);
    fnv_fold(got);
    total_checks++;
}

int main(void)
{
    /* Phase 1: exhaustive 16-bit x, both flag values: 131,072 checks. */
    for (uint32_t xv = 0; xv < 65536u; xv++) {
        uint64_t x = (uint64_t)xv;
        check_pair(x, 0ULL);
        check_pair(x, 1ULL);
    }
    printf("phase1 exhaustive-16bit: checks=%" PRIu64 ", mismatches=%" PRIu64 "\n",
           total_checks, mismatches);

    /* Phase 2: 10,000,000 fixed-seed splitmix64 (x, f) pairs, with the
     * flag drawn from the low bit of each odd rng output so the stream
     * covers the full 64-bit x space at both flag values. */
    {
        uint64_t before = total_checks;
        rng_state = 0x123456789ABCDEF0ULL;
        for (int i = 0; i < 10000000; i++)
            check_pair(rng_next(), rng_next() & 1ULL);
        printf("phase2 random: checks=%" PRIu64 ", mismatches=%" PRIu64 "\n",
               total_checks - before, mismatches);
    }

    /* Phase 3: flag x extreme rows printed, so both outcomes of the
     * conditional are pinned in the log. */
    {
        static const uint64_t edge_xs[6] = {
            0x0000000000000000ULL,
            0x0000000000000001ULL,
            0xFFFFFFFFFFFFFFFFULL,
            0x8000000000000000ULL,
            0x000000000000FFFFULL,
            0x123456789ABCDEF0ULL,
        };
        printf("phase3 edge rows (x, f, result):\n");
        for (int i = 0; i < 6; i++) {
            uint64_t r0 = cond_neg(edge_xs[i], 0ULL);
            uint64_t r1 = cond_neg(edge_xs[i], 1ULL);
            printf("  x=%016" PRIx64 " f=0 -> %016" PRIx64 "\n",
                   edge_xs[i], r0);
            printf("  x=%016" PRIx64 " f=1 -> %016" PRIx64 "\n",
                   edge_xs[i], r1);
            check_pair(edge_xs[i], 0ULL);
            check_pair(edge_xs[i], 1ULL);
        }
    }

    printf("total checks: %" PRIu64 "\n", total_checks);
    printf("mismatches: %" PRIu64 "\n", mismatches);
    printf("checksum: %016" PRIx64 "\n", fnv);
    printf(mismatches == 0 ? "PASS\n" : "FAIL\n");
    return (int)(mismatches != 0);
}
#endif
