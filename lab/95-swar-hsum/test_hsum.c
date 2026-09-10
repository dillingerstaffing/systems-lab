#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "hsum.h"

/*
 * Reference: scalar lane sum. Shifts out each 16-bit lane and adds
 * the four values. Every operation is unsigned, so the sum is exact.
 */
static uint32_t ref_hsum(uint64_t w)
{
    return (uint32_t)(((w >> 0)  & 0xFFFFULL) +
                      ((w >> 16) & 0xFFFFULL) +
                      ((w >> 32) & 0xFFFFULL) +
                      ((w >> 48) & 0xFFFFULL));
}

/* splitmix64: deterministic stream of test inputs. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;
static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 14695981039346656037ULL;
static void fnv(uint32_t v)
{
    fnv1a ^= (uint64_t)v;
    fnv1a *= 1099511628211ULL;
}

static unsigned long long mismatches = 0;
static unsigned long long checks = 0;

static void check(uint64_t w)
{
    uint32_t got = swar_hsum(w);
    uint32_t want = ref_hsum(w);
    if (got != want) {
        printf("MISMATCH w=%016llx got=%08x want=%08x\n",
               (unsigned long long)w, got, want);
        mismatches++;
    }
    fnv(got);
    checks++;
}

int main(void)
{
    /* 1. Directed rows, printed for the record. These cover the zero
       word, the all-ones word, each lane alone at maximum, the pair
       boundaries (lane 0 and lane 1 maxima that together exercise
       the stage-1 guard bound), alternating patterns, and the word
       where all lanes are 0x8000 (pair sums sit at exactly 0x10000,
       one past the 16-bit boundary, the sharpest guard-bound
       case). */
    static const struct { uint64_t w; } directed[] = {
        {0x0000000000000000ULL},
        {0xFFFFFFFFFFFFFFFFULL},
        {0x000000000000FFFFULL},
        {0x00000000FFFF0000ULL},
        {0x0000FFFF00000000ULL},
        {0xFFFF000000000000ULL},
        {0x00000000FFFFFFFFULL},
        {0xFFFFFFFF00000000ULL},
        {0x0001000100010001ULL},
        {0x8000800080008000ULL},
        {0xFFFEFFFEFFFEFFFEULL},
        {0x0001000200030004ULL},
        {0xA5A55A5AA5A55A5AULL},
        {0x123456789ABCDEF0ULL},
    };
    printf("directed rows:\n");
    for (size_t i = 0; i < sizeof(directed) / sizeof(directed[0]); i++) {
        uint64_t w = directed[i].w;
        uint32_t got = swar_hsum(w);
        uint32_t want = ref_hsum(w);
        printf("  w=%016llx swar_hsum=%08x ref=%08x %s\n",
               (unsigned long long)w, got, want,
               got == want ? "ok" : "FAIL");
        check(w);
    }

    /* 2. Exhaustive 16-bit lane pairs replicated to all lanes:
       words (a, b, a, b) for a, b in [0, 65535].
       2^32 = 4,294,967,296 words. */
    for (uint64_t a = 0; a <= 0xFFFF; a++) {
        uint64_t w = a | (a << 32);
        for (uint64_t b = 0; b <= 0xFFFF; b++)
            check(w | (b << 16) | (b << 48));
    }

    /* 3. 10,000,000 fixed-seed splitmix64 random 64-bit words. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 10000000; i++)
        check(splitmix64());

    printf("checks=%llu mismatches=%llu fnv1a=%016llx\n",
           checks, mismatches, (unsigned long long)fnv1a);

#ifdef BENCH
    /*
     * Throughput at -O2, best of 5. 1M words are generated once
     * before timing; the timed region is 25 passes over that array,
     * XOR-ing each swar_hsum result into a volatile sink. What is
     * measured is swar_hsum plus loop and memory traffic, not the
     * PRNG.
     */
    uint64_t *vals = malloc(1000000 * sizeof *vals);
    if (!vals) {
        printf("BENCH malloc failed\n");
        return 1;
    }
    rng_state = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < 1000000; i++)
        vals[i] = splitmix64();

    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint64_t sink = 0;
        for (int pass = 0; pass < 25; pass++)
            for (int i = 0; i < 1000000; i++)
                sink ^= swar_hsum(vals[i]);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    free(vals);
    printf("bench: %.2f ns/value (%.1f Mvalues/s over 25M timed values, best of 5)\n",
           best / 25000000.0, 25000000.0 / (best / 1e3));
#endif

    return mismatches == 0 ? 0 : 1;
}
