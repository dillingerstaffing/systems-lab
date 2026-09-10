#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "eqmask.h"

/* Reference: the naive per-byte loop. Bit i is 1 iff byte i of x
   equals byte i of y, where byte i is bits [8i, 8i+8). */
static uint8_t ref_eqmask(uint64_t x, uint64_t y)
{
    uint8_t m = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t xb = (uint8_t)(x >> (8 * i));
        uint8_t yb = (uint8_t)(y >> (8 * i));
        if (xb == yb)
            m |= (uint8_t)(1u << i);
    }
    return m;
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
static void fnv(uint64_t v)
{
    fnv1a ^= v;
    fnv1a *= 1099511628211ULL;
}

static unsigned long long mismatches = 0;
static unsigned long long checks = 0;

/* Differential check of swar_eqmask64 against the reference. */
static void check(uint64_t x, uint64_t y)
{
    uint8_t got = swar_eqmask64(x, y);
    uint8_t want = ref_eqmask(x, y);
    if (got != want) {
        printf("MISMATCH x=%016llx y=%016llx got=%02x want=%02x\n",
               (unsigned long long)x, (unsigned long long)y,
               (unsigned)got, (unsigned)want);
        mismatches++;
    }
    fnv(got);
    checks++;
}

int main(void)
{
    /* 1. Directed rows, printed for the record: all-equal,
       all-different, single-byte differences at each end and in the
       middle, the 0x80 high-bit boundary, and d = ONES itself (the
       adversarial input for the subtraction step). */
    static const struct { uint64_t x, y; } directed[] = {
        {0x0000000000000000ULL, 0x0000000000000000ULL},
        {0x0000000000000000ULL, 0x0000000000000001ULL},
        {0x0000000000000001ULL, 0x0000000000000000ULL},
        {0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL},
        {0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL},
        {0xFFFFFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFFFULL},
        {0x0102030405060708ULL, 0x0102030405060708ULL},
        {0x0102030405060708ULL, 0x0102030405060709ULL},
        {0x0102030405060708ULL, 0x0002030405060708ULL},
        {0x0102030405060708ULL, 0x0102030405FF0708ULL},
        {0x8080808080808080ULL, 0x8080808080808080ULL},
        {0x8080808080808080ULL, 0x0000000000000000ULL},
        {0x00FF00FF00FF00FFULL, 0x00FF00FF00FF00FEULL},
        {0x0101010101010101ULL, 0x0000000000000000ULL},
        {0x0101010101010101ULL, 0x0101010101010101ULL},
        {0x8000000000000000ULL, 0x7FFFFFFFFFFFFFFFULL},
    };
    printf("directed rows:\n");
    for (size_t i = 0; i < sizeof(directed) / sizeof(directed[0]); i++) {
        uint64_t x = directed[i].x, y = directed[i].y;
        uint8_t got = swar_eqmask64(x, y);
        uint8_t want = ref_eqmask(x, y);
        printf("  x=%016llx y=%016llx eqmask=%02x ref=%02x %s\n",
               (unsigned long long)x, (unsigned long long)y,
               (unsigned)got, (unsigned)want,
               got == want ? "ok" : "FAIL");
        check(x, y);
    }

    /* 2. Exhaustive 16-bit pairs: x and y over [0, 65535].
       65536 * 65536 = 4,294,967,296 = 2^32 pairs. */
    for (uint64_t x = 0; x <= 0xFFFF; x++)
        for (uint64_t y = 0; y <= 0xFFFF; y++)
            check(x, y);

    /* 3. 10,000,000 fixed-seed splitmix64 random 64-bit pairs. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 10000000; i++) {
        uint64_t x = splitmix64();
        uint64_t y = splitmix64();
        check(x, y);
    }

    printf("checks=%llu mismatches=%llu fnv1a=%016llx\n",
           checks, mismatches, (unsigned long long)fnv1a);

#ifdef BENCH
    /*
     * Throughput at -O2, best of 5. 1M pairs are generated once
     * before timing; the timed region is 25 passes over that array,
     * XOR-ing each swar_eqmask64 result into a volatile sink. What is
     * measured is swar_eqmask64 plus loop and memory traffic, not the
     * PRNG.
     */
    struct pair { uint64_t x, y; };
    struct pair *vals = malloc(1000000 * sizeof *vals);
    if (!vals) {
        printf("BENCH malloc failed\n");
        return 1;
    }
    rng_state = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < 1000000; i++) {
        vals[i].x = splitmix64();
        vals[i].y = splitmix64();
    }

    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint64_t sink = 0;
        for (int pass = 0; pass < 25; pass++)
            for (int i = 0; i < 1000000; i++)
                sink ^= (uint64_t)swar_eqmask64(vals[i].x, vals[i].y);
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
