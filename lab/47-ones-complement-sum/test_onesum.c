/*
 * test_onesum.c: verifies ones_complement_sum() three ways.
 *
 * 1. Known-answer vectors, each computed by hand (arithmetic shown in
 *    the code below): empty -> 0xFFFF; {0x00} -> 0xFFFF; {0xFF} ->
 *    0x00FF; four zero bytes -> 0xFFFF; four 0xFF bytes -> 0x0000
 *    (exercises end-around carry); {0x00,0x01,0xF2,0x03} -> 0x0DFB;
 *    {0xFF,0xFF,0x00,0x01} -> 0xFFFE (exercises carry out of the top
 *    bit folding back into bit 0); {0x01,0x02,0x03} -> 0xFBFD and
 *    {0x12,0x34,0x56} -> 0x97CB (both exercise odd-length zero padding).
 *    Each vector must also agree with the independent reference.
 * 2. Differential test against an independent naive reference over 1M
 *    fixed-seed random buffers. The reference shares no update logic
 *    with the implementation: it accumulates every 16-bit word into a
 *    64-bit total with no folding at all, then folds carries only once
 *    at the end (repeatedly, until none remain). The implementation
 *    folds the carry out of every single addition instead. If the two
 *    disagree anywhere, the run fails.
 * 3. Throughput benchmark at the compiled optimization level.
 *
 * An FNV-1a checksum is folded over every differential-test result so
 * that -O0, -O2, and sanitizer runs can be compared for bit-identical
 * output.
 *
 * Exit code 0 means every check passed; anything else is a failure.
 */
#define _POSIX_C_SOURCE 199309L /* for clock_gettime */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "onesum.h"

/* Deterministic PRNG: splitmix64, fixed stated seed. */
static unsigned long long rng_state = 0x123456789ABCDEF0ull;
static unsigned int rng_next(void)
{
    unsigned long long z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return (unsigned int)(z ^ (z >> 31));
}

/* FNV-1a 64-bit, used to fingerprint the full result stream. */
static unsigned long long fnv1a(unsigned long long h, unsigned long long v)
{
    h ^= v;
    h *= 1099511628211ull;
    return h;
}

/*
 * Independent reference: accumulate every word into a 64-bit total with
 * no per-addition folding, then fold all carries at the end. For len up
 * to 511 (the largest buffer below), the total is at most 256 * 0xFFFF
 * < 2^25, so the 64-bit accumulator cannot overflow before folding.
 */
static unsigned ref_onesum(const uint8_t *data, size_t len)
{
    unsigned long long acc = 0;
    size_t i = 0;

    while (i + 1 < len) {
        acc += ((unsigned long long)data[i] << 8) | data[i + 1];
        i += 2;
    }
    if (i < len)
        acc += (unsigned long long)data[i] << 8; /* odd pad, same as impl */
    while (acc >> 16)
        acc = (acc & 0xFFFFull) + (acc >> 16);
    return (unsigned)(~acc & 0xFFFFull);
}

static int check_vectors(void)
{
    static const uint8_t v_00[] = { 0x00 };
    static const uint8_t v_ff[] = { 0xFF };
    static const uint8_t v_zeros[] = { 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t v_ones[] = { 0xFF, 0xFF, 0xFF, 0xFF };
    static const uint8_t v_0102f203[] = { 0x00, 0x01, 0xF2, 0x03 };
    static const uint8_t v_ffff0001[] = { 0xFF, 0xFF, 0x00, 0x01 };
    static const uint8_t v_010203[] = { 0x01, 0x02, 0x03 };
    static const uint8_t v_123456[] = { 0x12, 0x34, 0x56 };
    struct vec {
        const char *desc;
        const uint8_t *data;
        size_t len;
        unsigned int expect;
    } vecs[9];

    /* Hand arithmetic, words are big-endian:
     * empty: no words, sum 0x0000, ~ = 0xFFFF.
     * {0x00}: one word 0x0000, sum 0x0000, ~ = 0xFFFF.
     * {0xFF}: one word 0xFF00, ~ = 0x00FF.
     * zeros: two words 0x0000, sum 0x0000, ~ = 0xFFFF.
     * ones: first word 0xFFFF -> acc 0xFFFF (no carry); second word:
     *       0xFFFF + 0xFFFF = 0x1FFFE, carry out 1, fold back:
     *       0xFFFE + 1 = 0xFFFF, ~ = 0x0000.
     * {0x00,0x01,0xF2,0x03}: words 0x0001, 0xF203;
     *       0x0001 + 0xF203 = 0xF204, ~ = 0x0DFB.
     * {0xFF,0xFF,0x00,0x01}: words 0xFFFF, 0x0001;
     *       0xFFFF + 0x0001 = 0x10000, carry out 1, fold back:
     *       0x0000 + 1 = 0x0001, ~ = 0xFFFE.
     * {0x01,0x02,0x03}: words 0x0102, 0x0300 (odd byte padded);
     *       0x0102 + 0x0300 = 0x0402, ~ = 0xFBFD.
     * {0x12,0x34,0x56}: words 0x1234, 0x5600 (odd byte padded);
     *       0x1234 + 0x5600 = 0x6834, ~ = 0x97CB. */
    vecs[0] = (struct vec){"empty", NULL, 0, 0xFFFFu};
    vecs[1] = (struct vec){"{0x00}", v_00, 1, 0xFFFFu};
    vecs[2] = (struct vec){"{0xFF}", v_ff, 1, 0x00FFu};
    vecs[3] = (struct vec){"zeros4", v_zeros, 4, 0xFFFFu};
    vecs[4] = (struct vec){"ones4", v_ones, 4, 0x0000u};
    vecs[5] = (struct vec){"{0x00,0x01,0xF2,0x03}", v_0102f203, 4, 0x0DFBu};
    vecs[6] = (struct vec){"{0xFF,0xFF,0x00,0x01}", v_ffff0001, 4, 0xFFFEu};
    vecs[7] = (struct vec){"{0x01,0x02,0x03}", v_010203, 3, 0xFBFDu};
    vecs[8] = (struct vec){"{0x12,0x34,0x56}", v_123456, 3, 0x97CBu};

    int fails = 0;
    for (int i = 0; i < 9; i++) {
        unsigned int got = ones_complement_sum(vecs[i].data, vecs[i].len);
        unsigned int ref = ref_onesum(vecs[i].data, vecs[i].len);
        int ok = (got == vecs[i].expect) && (got == ref);
        printf("vector %-22s expect=0x%04x got=0x%04x ref=0x%04x %s\n",
               vecs[i].desc, vecs[i].expect, got, ref,
               ok ? "OK" : "FAIL");
        fails += !ok;
    }
    return fails;
}

/* Largest buffer any length class below can produce. */
#define MAXLEN 511

static int differential_test(void)
{
    static uint8_t buf[MAXLEN];
    unsigned long long mismatches = 0;
    unsigned long long total_bytes = 0;
    unsigned long long h = 14695981039346656037ull;
    const unsigned long long N = 1000000ull;

    for (unsigned long long t = 0; t < N; t++) {
        size_t len;
        switch (rng_next() % 10) {
        case 0:
            len = 0;                            /* empty */
            break;
        case 1: case 2: case 3:
            len = rng_next() % 64;              /* tiny, incl. odd */
            break;
        case 4: case 5: case 6:
            len = 1 + (rng_next() % 511);       /* 1..511, full mix */
            break;
        default:
            len = 64 + (rng_next() % 448);      /* 64..511 */
            break;
        }
        for (size_t i = 0; i < len; i++)
            buf[i] = (uint8_t)rng_next();
        total_bytes += len;

        unsigned int got = ones_complement_sum(buf, len);
        unsigned int want = ref_onesum(buf, len);
        h = fnv1a(h, got);
        if (got != want) {
            if (mismatches < 5)
                printf("MISMATCH len=%zu got=0x%04x want=0x%04x\n",
                       len, got, want);
            mismatches++;
        }
    }

    printf("differential: %llu buffers, %llu bytes total, "
           "seed=0x123456789ABCDEF0, mismatches=%llu\n",
           N, total_bytes, mismatches);
    printf("fnv1a=0x%016llx\n", h);
    return mismatches != 0;
}

static int benchmark(void)
{
    static uint8_t bench[16 * 1024 * 1024]; /* 16 MiB */
    for (size_t i = 0; i < sizeof bench; i++)
        bench[i] = (uint8_t)(rng_next() & 0xff);

    const unsigned long long target = 256ull * 1024 * 1024; /* 256 MiB */
    unsigned long long done = 0;
    unsigned int acc = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (done < target) {
        acc ^= ones_complement_sum(bench, sizeof bench);
        done += sizeof bench;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double secs = (t1.tv_sec - t0.tv_sec) +
                  (t1.tv_nsec - t0.tv_nsec) / 1e9;
    double mib = (double)done / (1024.0 * 1024.0);
    printf("throughput: %.1f MiB in %.3f s = %.1f MiB/s (checksum folded: "
           "0x%08x, printed so the loop is not dead code)\n",
           mib, secs, mib / secs, acc);
    return secs <= 0.0;
}

int main(void)
{
    int fails = 0;
    fails += check_vectors();
    fails += differential_test();
    fails += benchmark();
    printf(fails ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", fails);
    return fails != 0;
}
