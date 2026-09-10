/*
 * Differential test for lab/56-bit-rotate-add.
 *
 * rotl64 is checked against an independent per-bit reference that moves
 * each set bit to its rotated position individually, deliberately not
 * using the shift/OR rotate identity that rotl64 itself uses.
 * The reference also yields the right rotation, so every case checks
 * the invariant rotl(rotr(x, k), k) == x with the two functions coming
 * from different constructions.
 *
 * Coverage: directed hand-computed cases, then all 64 rotation amounts
 * k = 0..63 crossed with 1,000,000 fixed-seed splitmix64 64-bit values
 * each (seed 0x9E3779B97F4A7C15, reset per k so every amount sees the
 * same 1,000,000 values; fully reproducible). Every case runs three
 * checks: rotl64 == reference, rotl64(ref_rotr(x,k),k) == x, and
 * rot_add64 == reference rotate plus wrapping unsigned add.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "rot_add.h"

#define SEED 0x9E3779B97F4A7C15ULL
#define NVALUES 1000000UL
#define NPERF 100000000UL

static uint64_t rng_state;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Per-bit reference: walk all 64 positions, move each set bit to its
   rotated slot. Produces both left and right rotations in one pass. */
static void ref_rot(uint64_t x, unsigned k, uint64_t *rl, uint64_t *rr)
{
    uint64_t l = 0, r = 0;
    unsigned i;

    k %= 64u;
    for (i = 0; i < 64u; i++) {
        if ((x >> i) & 1u) {
            l |= 1ULL << ((i + k) % 64u);
            r |= 1ULL << ((i + 64u - k) % 64u);
        }
    }
    *rl = l;
    *rr = r;
}

static uint64_t fnv1a = 14695981039346656037ULL;

static void checksum_u64(uint64_t v)
{
    fnv1a ^= v;
    fnv1a *= 1099511628211ULL;
}

static int fails = 0;

static void expect_u64(const char *name, uint64_t got, uint64_t want)
{
    if (got != want) {
        printf("FAIL %s: got 0x%016" PRIx64 " want 0x%016" PRIx64 "\n",
               name, got, want);
        fails++;
    }
}

int main(void)
{
    const uint64_t x = 0x0123456789ABCDEFULL;
    unsigned long k, i;
    unsigned long rotl_mism = 0, ident_mism = 0, rotadd_mism = 0;
    unsigned long cases = 0;

    /* Directed cases, answers hand-computed. */
    expect_u64("rotl(x,0)", rotl64(x, 0), x);
    expect_u64("rotl(x,64)", rotl64(x, 64), x);
    expect_u64("rotl(x,128)", rotl64(x, 128), x);
    expect_u64("rotl(x,65)==rotl(x,1)", rotl64(x, 65), rotl64(x, 1));
    expect_u64("rotl(x,1)", rotl64(x, 1), 0x02468ACF13579BDEULL);
    expect_u64("rotl(x,63)", rotl64(x, 63), 0x8091A2B3C4D5E6F7ULL);
    expect_u64("rotl(0x8000..,1)", rotl64(0x8000000000000000ULL, 1), 1ULL);
    expect_u64("rot_add(x,FF..FF,1)", rot_add64(x, 0xFFFFFFFFFFFFFFFFULL, 1),
               0x02468ACF13579BDDULL);
    expect_u64("rot_add(FF..FF,1,0) wraps",
               rot_add64(0xFFFFFFFFFFFFFFFFULL, 1, 0), 0ULL);
    expect_u64("rot_add(0,FF..FF,7)", rot_add64(0, 0xFFFFFFFFFFFFFFFFULL, 7),
               0xFFFFFFFFFFFFFFFFULL);
    printf("directed: %s\n", fails ? "FAIL" : "10/10 OK");

    /* Differential sweep. */
    for (k = 0; k < 64; k++) {
        rng_state = SEED;
        for (i = 0; i < NVALUES; i++) {
            uint64_t xv = splitmix64();
            uint64_t yv = splitmix64();
            uint64_t r = rotl64(xv, (unsigned)k);
            uint64_t rl, rr, ra;

            ref_rot(xv, (unsigned)k, &rl, &rr);
            if (r != rl) {
                if (rotl_mism < 5)
                    printf("rotl mismatch: k=%lu x=0x%016" PRIx64
                           " got=0x%016" PRIx64 " ref=0x%016" PRIx64 "\n",
                           k, xv, r, rl);
                rotl_mism++;
            }
            if (rotl64(rr, (unsigned)k) != xv)
                ident_mism++;
            ra = rot_add64(xv, yv, (unsigned)k);
            if (ra != (uint64_t)(rl + yv)) {
                if (rotadd_mism < 5)
                    printf("rot_add mismatch: k=%lu x=0x%016" PRIx64
                           " y=0x%016" PRIx64 "\n", k, xv, yv);
                rotadd_mism++;
            }
            checksum_u64(ra);
            cases++;
        }
    }
    printf("differential: amounts=64 values_per_amount=%lu cases=%lu "
           "rotl_mism=%lu ident_mism=%lu rotadd_mism=%lu\n",
           NVALUES, cases, rotl_mism, ident_mism, rotadd_mism);
    printf("checksum=%" PRIu64 "\n", fnv1a);

#ifdef PERF_TEST
    {
        struct timespec t0, t1;
        uint64_t sink = 0;
        unsigned long long ns_total;
        double ns_per;

        rng_state = SEED;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (i = 0; i < NPERF; i++) {
            uint64_t xv = splitmix64();
            sink += rot_add64(xv, splitmix64(), 41);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        ns_total = (unsigned long long)(t1.tv_sec - t0.tv_sec) * 1000000000ULL
                 + (unsigned long long)(t1.tv_nsec - t0.tv_nsec);
        ns_per = (double)ns_total / (double)NPERF;
        printf("throughput: values=%lu ns_total=%llu ns_per_value=%.2f "
               "sink=%" PRIu64 "\n", NPERF, ns_total, ns_per, sink);
    }
#endif

    if (fails || rotl_mism || ident_mism || rotadd_mism) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
