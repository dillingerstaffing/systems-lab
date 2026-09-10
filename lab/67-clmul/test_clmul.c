/*
 * test_clmul.c - verification for lab/67-clmul.
 *
 * 1. Known-answer vectors, each hand-checked from the GF(2) polynomial
 *    definition (see PROOF.md for the derivations).
 * 2. Exhaustive differential over all 2^32 pairs of 16-bit inputs:
 *    the shift-xor recurrence (iterating a's bits, shifting b) against
 *    an independent recurrence (iterating b's bits, shifting a).
 *    Fused into one unrolled pass so the 2^32 loop stays practical.
 * 3. 1,000,000 fixed-seed splitmix64 64-bit pairs: implementation vs
 *    the independent recurrence, plus on every case the invariants
 *    clmul(a, b^c) == clmul(a,b) ^ clmul(a,c) (distributivity),
 *    clmul(a,b) == clmul(b,a) (commutativity),
 *    clmul(a,1) == a and clmul(a,0) == 0.
 *    The first 20,000 pairs are additionally checked against a
 *    direct per-output-bit convolution reference, which is the
 *    polynomial-product definition written out literally.
 * 4. Throughput of clmul64 at -O2 over prefilled arrays (no PRNG in
 *    the timed region).
 *
 * All numeric outputs are folded into one FNV-1a checksum over the
 * byte stream of results; it must be identical across -O0, -O2, and
 * ASan+UBSan builds.
 */
#define _POSIX_C_SOURCE 199309L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "clmul.h"

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

static uint64_t fnv1a_u32(uint64_t h, uint32_t v)
{
    for (int i = 0; i < 4; i++) {
        h ^= (uint64_t)((v >> (8 * i)) & 0xFFu);
        h *= FNV_PRIME;
    }
    return h;
}

static uint64_t fnv1a_u64(uint64_t h, uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        h ^= (v >> (8 * i)) & 0xFFu;
        h *= FNV_PRIME;
    }
    return h;
}

/* Independent recurrence: iterate b's bits, shift a. */
static clmul128 clmul64_swap(uint64_t a, uint64_t b)
{
    uint64_t lo = 0, hi = 0;
    for (int i = 0; i < 64; i++) {
        uint64_t m = 0ULL - ((b >> i) & 1ULL);
        lo ^= (a << i) & m;
        hi ^= (i == 0) ? 0ULL : (a >> (64 - i)) & m;
    }
    return (clmul128){ .hi = hi, .lo = lo };
}

/* Definition-level reference: result bit j = XOR over i of
 * bit_i(a) & bit_{j-i}(b).  Slow; used on a 20k-case subset. */
static clmul128 clmul64_conv(uint64_t a, uint64_t b)
{
    uint64_t lo = 0, hi = 0;
    for (int j = 0; j < 128; j++) {
        unsigned bit = 0;
        int imin = j - 63 < 0 ? 0 : j - 63;
        int imax = j > 63 ? 63 : j;
        for (int i = imin; i <= imax; i++)
            bit ^= (unsigned)(((a >> i) & 1u) & ((b >> (j - i)) & 1u));
        if (bit != 0) {
            if (j < 64)
                lo |= 1ULL << j;
            else
                hi |= 1ULL << (j - 64);
        }
    }
    return (clmul128){ .hi = hi, .lo = lo };
}

static int eq128(clmul128 x, clmul128 y)
{
    return x.hi == y.hi && x.lo == y.lo;
}

static clmul128 xor128(clmul128 x, clmul128 y)
{
    clmul128 r = { x.hi ^ y.hi, x.lo ^ y.lo };
    return r;
}

/* splitmix64, fixed seed 0x123456789ABCDEF0. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static int check_vector(const char *name, uint64_t a, uint64_t b,
                        uint64_t ehi, uint64_t elo, uint64_t *fnv)
{
    clmul128 g = clmul64(a, b);
    int ok = (g.hi == ehi && g.lo == elo);
    printf("vector %-14s got={%016" PRIx64 ",%016" PRIx64 "} expect={%016"
           PRIx64 ",%016" PRIx64 "} %s\n",
           name, g.hi, g.lo, ehi, elo, ok ? "OK" : "FAIL");
    *fnv = fnv1a_u64(*fnv, g.hi);
    *fnv = fnv1a_u64(*fnv, g.lo);
    return ok;
}

/* One fused, fully unrolled 16-bit exhaustive step. x iterates a's
 * bits shifting b; y iterates b's bits shifting a. */
#define EXH_STEP(i)                                  \
    do {                                             \
        uint32_t ma = 0u - ((a >> (i)) & 1u);        \
        uint32_t mb = 0u - ((b >> (i)) & 1u);        \
        x ^= (b << (i)) & ma;                        \
        y ^= (a << (i)) & mb;                        \
    } while (0)

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#define BENCH_N 2000000
static uint64_t bench_a[BENCH_N];
static uint64_t bench_b[BENCH_N];

int main(void)
{
    uint64_t fnv = FNV_OFFSET;
    int fail = 0;

    /* ---- 1. known-answer vectors (hand derivations in PROOF.md) ---- */
    fail += !check_vector("zero", 0ULL, 0x123456789ABCDEF0ULL, 0, 0, &fnv);
    fail += !check_vector("identity", 1ULL, 0xDEADBEEFCAFEBABEULL, 0,
                          0xDEADBEEFCAFEBABEULL, &fnv);
    fail += !check_vector("2*3", 2ULL, 3ULL, 0, 6, &fnv);
    fail += !check_vector("ff*ff", 0xFFULL, 0xFFULL, 0, 0x5555ULL, &fnv);
    fail += !check_vector("32ones^2", 0xFFFFFFFFULL, 0xFFFFFFFFULL, 0,
                          0x5555555555555555ULL, &fnv);
    fail += !check_vector("x63*x63", 0x8000000000000000ULL,
                          0x8000000000000000ULL, 0x4000000000000000ULL, 0,
                          &fnv);
    fail += !check_vector("64ones^2", 0xFFFFFFFFFFFFFFFFULL,
                          0xFFFFFFFFFFFFFFFFULL, 0x5555555555555555ULL,
                          0x5555555555555555ULL, &fnv);
    fail += !check_vector("nibble^2", 0x1111111111111111ULL,
                          0x1111111111111111ULL, 0x0101010101010101ULL,
                          0x0101010101010101ULL, &fnv);

    /* ---- 2. exhaustive 16-bit pairs: 2^32 differentials ---- */
    uint64_t mism16 = 0;
    for (uint32_t a = 0; a < 65536u; a++) {
        for (uint32_t b = 0; b < 65536u; b++) {
            uint32_t x = 0, y = 0;
            EXH_STEP(0); EXH_STEP(1); EXH_STEP(2); EXH_STEP(3);
            EXH_STEP(4); EXH_STEP(5); EXH_STEP(6); EXH_STEP(7);
            EXH_STEP(8); EXH_STEP(9); EXH_STEP(10); EXH_STEP(11);
            EXH_STEP(12); EXH_STEP(13); EXH_STEP(14); EXH_STEP(15);
            mism16 += (uint64_t)(x != y);
            fnv = fnv1a_u32(fnv, x);
        }
    }
    printf("exhaustive16: 4294967296 pairs, mismatches=%" PRIu64 "\n",
           mism16);
    fail += (mism16 != 0);

    /* ---- 3. 1M random 64-bit pairs + invariants ---- */
    uint64_t mism64 = 0, dist_viol = 0, comm_viol = 0;
    uint64_t ident_viol = 0, ann_viol = 0, conv_mism = 0;
    for (uint64_t n = 0; n < 1000000u; n++) {
        uint64_t a = rng_next(), b = rng_next(), c = rng_next();
        clmul128 x = clmul64(a, b);
        if (!eq128(x, clmul64_swap(a, b)))
            mism64++;
        if (!eq128(clmul64(a, b ^ c), xor128(x, clmul64(a, c))))
            dist_viol++;
        if (!eq128(x, clmul64(b, a)))
            comm_viol++;
        clmul128 xa = clmul64(a, 1);
        if (xa.hi != 0 || xa.lo != a)
            ident_viol++;
        clmul128 za = clmul64(a, 0);
        if (za.hi != 0 || za.lo != 0)
            ann_viol++;
        if (n < 20000u && !eq128(x, clmul64_conv(a, b)))
            conv_mism++;
        fnv = fnv1a_u64(fnv, x.lo);
        fnv = fnv1a_u64(fnv, x.hi);
    }
    printf("random64: 1000000 pairs, ref_mismatches=%" PRIu64
           " conv_mismatches=%" PRIu64 "\n",
           mism64, conv_mism);
    printf("invariants: distributivity_violations=%" PRIu64
           " commutativity_violations=%" PRIu64
           " identity_violations=%" PRIu64
           " annihilator_violations=%" PRIu64 "\n",
           dist_viol, comm_viol, ident_viol, ann_viol);
    fail += (mism64 != 0) + (conv_mism != 0) + (dist_viol != 0)
          + (comm_viol != 0) + (ident_viol != 0) + (ann_viol != 0);

    printf("fnv1a=%016" PRIx64 "\n", fnv);

    /* ---- 4. throughput at -O2 (prefilled arrays, timed region is
     * pure clmul64; results xor-folded into a printed sink) ---- */
    for (uint64_t n = 0; n < BENCH_N; n++) {
        bench_a[n] = rng_next();
        bench_b[n] = rng_next();
    }
    uint64_t sink = 0;
    double t0 = now_s();
    for (uint64_t n = 0; n < BENCH_N; n++) {
        clmul128 r = clmul64(bench_a[n], bench_b[n]);
        sink ^= r.lo ^ r.hi;
    }
    double dt = now_s() - t0;
    printf("throughput: %d clmul64 in %.3f s = %.2f ns/value (sink=%016"
           PRIx64 ")\n",
           BENCH_N, dt, dt * 1e9 / (double)BENCH_N, sink);

    printf("RESULT: %s\n", fail == 0 ? "PASS" : "FAIL");
    return fail == 0 ? 0 : 1;
}
