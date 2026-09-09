#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "hamming.h"

/* Fixed-seed splitmix64: reproducible stream of 64-bit values. */
static uint64_t rng_state;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Naive bit-loop reference: count the set bits of a ^ b one at a time.
 * Independent of the SWAR construction; the only shared identity is
 * the definition of XOR. */
static uint32_t ref_hamming64(uint64_t a, uint64_t b)
{
    uint64_t w = a ^ b;
    uint32_t n = 0;
    while (w != 0) {
        n += (uint32_t)(w & 1ULL);
        w >>= 1;
    }
    return n;
}

/* Differential check: hamming64 vs the naive loop, plus the symmetry
 * invariant d(a,b) == d(b,a) (XOR is commutative). Returns 1 on any
 * mismatch, 0 when both agree. */
static int check(uint64_t a, uint64_t b)
{
    uint32_t got = hamming64(a, b);
    uint32_t want = ref_hamming64(a, b);
    if (got != want)
        return 1;
    if (hamming64(a, b) != hamming64(b, a))
        return 1;
    return 0;
}

#define N_RANDOM 10000000UL

int main(void)
{
    unsigned long mismatches = 0;
    unsigned long cases = 0;
    /* FNV-1a 64 over the distance values; identical only if every
     * comparison ran the same deterministic stream with the same
     * answers, so the compiler cannot skip the work. */
    uint64_t fnv = 14695981039346656037ULL;

    /* 1. Directed edge cases. */
    static const struct { uint64_t a, b; uint32_t expect; } directed[] = {
        { 0x0000000000000000ULL, 0x0000000000000000ULL, 0 },  /* equal -> 0 */
        { 0x0123456789ABCDEFULL, 0x0123456789ABCDEFULL, 0 }, /* equal -> 0 */
        { 0xFFFFFFFFFFFFFFFFULL, 0x0000000000000000ULL, 64 }, /* all-ones vs zero -> 64 */
        { 0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL, 64 }, /* alternating opposites -> 64 */
        { 0x5555555555555555ULL, 0x5555555555555555ULL, 0 },  /* alternating self -> 0 */
        { 0x0000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL, 64 }, /* d(x, ~x) == 64 */
        { 0x0000000000000001ULL, 0xFFFFFFFFFFFFFFFEULL, 64 },
        { 0xDEADBEEFCAFEBABEULL, 0x2152411035014541ULL, 64 },
        { 0x8000000000000000ULL, 0x7FFFFFFFFFFFFFFFULL, 64 },
    };
    for (size_t i = 0; i < sizeof(directed) / sizeof(directed[0]); i++) {
        uint32_t got = hamming64(directed[i].a, directed[i].b);
        if (got != directed[i].expect || check(directed[i].a, directed[i].b))
            mismatches++;
        fnv ^= got;
        fnv *= 1099511628211ULL;
        cases++;
    }

    /* Single-bit differences: flipping exactly one bit of the base word
     * must give distance 1, for every bit position 0..63. */
    const uint64_t base = 0x0123456789ABCDEFULL;
    for (int b = 0; b < 64; b++) {
        uint32_t got = hamming64(base, base ^ (1ULL << b));
        if (got != 1 || check(base, base ^ (1ULL << b)))
            mismatches++;
        fnv ^= got;
        fnv *= 1099511628211ULL;
        cases++;
    }

    /* 2. Random differential: 10,000,000 fixed-seed pairs. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (unsigned long i = 0; i < N_RANDOM; i++) {
        uint64_t a = rng_next();
        uint64_t b = rng_next();
        uint32_t got = hamming64(a, b);
        if (check(a, b))
            mismatches++;
        fnv ^= got;
        fnv *= 1099511628211ULL;
    }
    cases += N_RANDOM;

    printf("total_cases=%lu mismatches=%lu fnv1a=%016llx\n",
           cases, mismatches, (unsigned long long)fnv);

    /* 3. Throughput: timed loop over fresh fixed-seed pairs. */
    uint64_t sink = 0;
    rng_state = 0xFEDCBA9876543210ULL;
    struct timespec t0, t1;
    const unsigned long N_TIMED = 100000000UL;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (unsigned long i = 0; i < N_TIMED; i++) {
        sink += hamming64(rng_next(), rng_next());
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ns = (double)(t1.tv_sec - t0.tv_sec) * 1e9 +
                (double)(t1.tv_nsec - t0.tv_nsec);
    printf("timed_pairs=%lu ns_total=%.0f ns_per_pair=%.2f Mpairs_per_sec=%.1f\n",
           N_TIMED, ns, ns / (double)N_TIMED,
           (double)N_TIMED / ns * 1e3);
    printf("sink=%llu\n", (unsigned long long)sink);

    if (mismatches != 0)
        return 1;
    printf("PASS\n");
    return 0;
}
