#define _POSIX_C_SOURCE 200809L
/*
 * Differential test for pdep64 (lab/116-pdep-emul).
 *
 * The oracle is an independent naive per-bit loop: it walks bit
 * positions 0..63 in order, and whenever the mask has that bit set it
 * deposits the next src bit there. That is a different algorithm from
 * the implementation's while-loop over set bits, so agreement pins the
 * scatter identity rather than a shared bug.
 *
 * With -DBENCH the correctness phases are skipped and throughput is
 * measured instead.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef BENCH
#include "pdep.c"
#else
#include "pdep.h"
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
            sink ^= pdep64(rng_next(), rng_next());
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

/* Naive reference: position-ordered scatter, structurally different
 * from the implementation's set-bit walk. Builtins are fine here; the
 * oracle is allowed every tool, the implementation is not. */
static uint64_t oracle_pdep(uint64_t src, uint64_t mask)
{
    uint64_t result = 0;
    unsigned j = 0;
    for (unsigned i = 0; i < 64; i++) {
        if ((mask >> i) & 1u) {
            if ((src >> j) & 1u)
                result |= 1ULL << i;
            j++;
        }
    }
    return result;
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

/*
 * Conservation invariant: the deposit moves exactly the low
 * popcount(mask) bits of src, so the result must contain exactly those
 * bits and no others:
 *
 *   popcount(result) == popcount(src & lowmask)
 *
 * where lowmask has the low popcount(mask) bits set (the k == 64 edge
 * is written without a 64-bit shift, which would be UB).
 */
static int check_invariant(uint64_t src, uint64_t mask, uint64_t result)
{
    unsigned k = (unsigned)__builtin_popcountll(mask);
    uint64_t lowmask = (k == 64) ? ~0ULL : ((1ULL << k) - 1u);
    unsigned want = (unsigned)__builtin_popcountll(src & lowmask);
    unsigned got = (unsigned)__builtin_popcountll(result);
    return (int)(got == want);
}

static uint64_t mismatches = 0;
static uint64_t total_pairs = 0;

static void check_pair(uint64_t src, uint64_t mask)
{
    uint64_t got = pdep64(src, mask);
    uint64_t want = oracle_pdep(src, mask);
    mismatches += (uint64_t)((got != want) ||
                            !check_invariant(src, mask, got));
    fnv_fold(got);
    total_pairs++;
}

int main(void)
{
    /* Phase 1: 1,000,000 fixed-seed splitmix64 pairs. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 1000000; i++)
        check_pair(rng_next(), rng_next());
    printf("phase1 random: pairs=%" PRIu64 ", mismatches=%" PRIu64 "\n",
           total_pairs, mismatches);

    /* Phase 2: exhaustive 8-bit masks (all 256 values in the low byte)
     * against 8 directed src values chosen to hit alternating runs,
     * hex-pattern carry textures, byte extremes, and a top-bit row. */
    {
        static const uint64_t srcs[8] = {
            0x0000000000000000ULL,
            0xFFFFFFFFFFFFFFFFULL,
            0xAAAAAAAAAAAAAAAAULL,
            0x5555555555555555ULL,
            0x0123456789ABCDEFULL,
            0xFEDCBA9876543210ULL,
            0x00000000000000FFULL,
            0x8000000000000000ULL,
        };
        uint64_t before = total_pairs;
        for (unsigned mask = 0; mask < 256; mask++)
            for (int s = 0; s < 8; s++)
                check_pair(srcs[s], (uint64_t)mask);
        printf("phase2 exhaustive-8bit: pairs=%" PRIu64 ", mismatches=%" PRIu64 "\n",
               total_pairs - before, mismatches);
    }

    /* Phase 3: mask=0 and mask=all-ones edge rows, printed as a table
     * so the extreme scatter behavior is pinned in the log. */
    {
        static const uint64_t edge_srcs[6] = {
            0x0000000000000000ULL,
            0xDEADBEEFCAFEBABEULL,
            0xFFFFFFFFFFFFFFFFULL,
            0xAAAAAAAAAAAAAAAAULL,
            0x5555555555555555ULL,
            0x8000000000000001ULL,
        };
        printf("phase3 edge rows (src, mask, result):\n");
        for (int s = 0; s < 6; s++) {
            uint64_t r0 = pdep64(edge_srcs[s], 0ULL);
            uint64_t r1 = pdep64(edge_srcs[s], ~0ULL);
            printf("  src=%016" PRIx64 " mask=%016" PRIx64 " -> %016" PRIx64 "\n",
                   edge_srcs[s], (uint64_t)0, r0);
            printf("  src=%016" PRIx64 " mask=%016" PRIx64 " -> %016" PRIx64 "\n",
                   edge_srcs[s], ~UINT64_C(0), r1);
            check_pair(edge_srcs[s], 0ULL);
            check_pair(edge_srcs[s], ~0ULL);
        }
    }

    printf("total pairs: %" PRIu64 "\n", total_pairs);
    printf("mismatches: %" PRIu64 "\n", mismatches);
    printf("checksum: %016" PRIx64 "\n", fnv);
    printf(mismatches == 0 ? "PASS\n" : "FAIL\n");
    return (int)(mismatches != 0);
}
#endif
