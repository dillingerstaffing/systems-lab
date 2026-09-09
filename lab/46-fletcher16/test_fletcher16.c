/*
 * test_fletcher16.c: verifies fletcher16() three ways.
 *
 * 1. Known-answer vectors: empty -> 0x0000, plus the published test
 *    vectors from the Fletcher's checksum article ("abcde" -> 0xC8F0,
 *    "abcdef" -> 0x2057, "abcdefgh" -> 0x0627), plus three hand-computed
 *    small cases ("a" -> 0x6161, {0xFF} -> 0x0000, {0x01,0xFF} -> 0x0201;
 *    arithmetic shown in the code below), plus "123456789" -> 0x1EDE,
 *    verified by hand arithmetic (sum1 = 477 % 255 = 222, sum2 = 2325 %
 *    255 = 30). Each vector must also agree with the independent
 *    reference.
 * 2. Differential test against an independent closed-form reference over
 *    1M fixed-seed random buffers. The reference shares no update logic
 *    with fletcher16(): it computes sum1 = (sum of bytes) mod 255 and
 *    sum2 = (sum over bytes of (n - k) * byte[k]) mod 255 in two
 *    separate loops. (This closed form equals the recurrence: by
 *    induction on the loop index, sum2 after processing n bytes is the
 *    sum of every byte weighted by how many remaining steps add it.)
 * 3. Throughput benchmark at the compiled optimization level.
 *
 * An FNV-1a checksum is folded over every differential-test result so
 * that -O0, -O2, and ASan+UBSan runs can be compared for bit-identical
 * output.
 *
 * Exit code 0 means every check passed; anything else is a failure.
 */
#define _POSIX_C_SOURCE 199309L /* for clock_gettime */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fletcher16.h"

/* Deterministic PRNG (splitmix64-style LCG). Seed is fixed and stated. */
static unsigned long long rng_state = 0xF1E7C416ull; /* the fixed seed */
static unsigned int rng_next(void)
{
    rng_state = rng_state * 6364136223846793005ull + 1442695040888963407ull;
    return (unsigned int)(rng_state >> 33);
}

/* FNV-1a 64-bit, used to fingerprint the full result stream. */
static unsigned long long fnv1a(unsigned long long h, unsigned long long v)
{
    h ^= v;
    h *= 1099511628211ull;
    return h;
}

/*
 * Independent reference: closed-form weighted sums, no recurrence.
 * sum1 = (sum of bytes) mod 255; sum2 = (sum of (n - k) * byte[k] over
 * k = 0..n-1) mod 255, where byte[0] is the first byte. 64-bit
 * accumulators hold the largest possible sums (n = 20016: byte sum at
 * most 20016*255 < 2^33, weighted sum at most 20016^2*255 < 2^59), so
 * nothing overflows before the final % 255.
 */
static unsigned ref_fletcher16(const unsigned char *data, size_t len)
{
    unsigned long long s1 = 0, s2 = 0;
    for (size_t k = 0; k < len; k++)
        s1 += data[k];
    for (size_t k = 0; k < len; k++)
        s2 += (unsigned long long)(len - k) * data[k];
    return (unsigned)(((s2 % 255ull) << 8) | (s1 % 255ull));
}

static int check_vectors(void)
{
    static const unsigned char v_abcde[] = "abcde";
    static const unsigned char v_abcdef[] = "abcdef";
    static const unsigned char v_abcdefgh[] = "abcdefgh";
    static const unsigned char v_123456789[] = "123456789";
    static const unsigned char v_a[] = "a";
    static const unsigned char v_ff[] = { 0xFF };
    static const unsigned char v_01ff[] = { 0x01, 0xFF };
    struct vec {
        const char *desc;
        const unsigned char *data;
        size_t len;
        unsigned int expect;
    } vecs[8];

    /* "abcde"/"abcdef"/"abcdefgh": the published Fletcher-16 test
     * vectors from the Fletcher's checksum article (0xC8F0, 0x2057,
     * 0x0627), each also re-checked by hand:
     * "abcde": s1 = 97+98+99+100+101 = 495 % 255 = 240,
     *          s2 = 5*97+4*98+3*99+2*100+1*101 = 1475 % 255 = 200,
     *          -> 0xC8F0.
     * "abcdef": s1 = (240+102) % 255 = 87, s2 = (200+87) % 255 = 32,
     *           -> 0x2057.
     * "abcdefgh": s1 = (87+103) % 255 = 190, s2 = (32+190) % 255 = 222,
     *             then s1 = (190+104) % 255 = 39, s2 = (222+39) % 255 = 6,
     *             -> 0x0627.
     * "123456789": s1 = 477 % 255 = 222 = 0xDE,
     *              s2 = (9*49+8*50+7*51+6*52+5*53+4*54+3*55+2*56+57)
     *                 = 2325 % 255 = 30 = 0x1E, -> 0x1EDE.
     * "a": s1 = (0+97)%255 = 97, s2 = (0+97)%255 = 97 -> 0x6161.
     * {0xFF}: s1 = (0+255)%255 = 0, s2 = (0+0)%255 = 0 -> 0x0000.
     * {0x01,0xFF}: s1 = 1, then (1+255)%255 = 1; s2 = 1, then
     *              (1+1)%255 = 2 -> 0x0201. */
    vecs[0] = (struct vec){"empty", NULL, 0, 0x0000u};
    vecs[1] = (struct vec){"\"abcde\"", v_abcde, 5, 0xC8F0u};
    vecs[2] = (struct vec){"\"abcdef\"", v_abcdef, 6, 0x2057u};
    vecs[3] = (struct vec){"\"abcdefgh\"", v_abcdefgh, 8, 0x0627u};
    vecs[4] = (struct vec){"\"123456789\"", v_123456789, 9, 0x1EDEu};
    vecs[5] = (struct vec){"\"a\"", v_a, 1, 0x6161u};
    vecs[6] = (struct vec){"{0xFF}", v_ff, 1, 0x0000u};
    vecs[7] = (struct vec){"{0x01,0xFF}", v_01ff, 2, 0x0201u};

    int fails = 0;
    for (int i = 0; i < 8; i++) {
        unsigned int got = fletcher16(vecs[i].data, vecs[i].len);
        unsigned int ref = ref_fletcher16(vecs[i].data, vecs[i].len);
        int ok = (got == vecs[i].expect) && (got == ref);
        printf("vector %-14s expect=0x%04x got=0x%04x ref=0x%04x %s\n",
               vecs[i].desc, vecs[i].expect, got, ref,
               ok ? "OK" : "FAIL");
        fails += !ok;
    }
    return fails;
}

/* Largest buffer any length class below can produce. */
#define MAXLEN 20016

static int differential_test(void)
{
    static unsigned char buf[MAXLEN];
    unsigned long long mismatches = 0;
    unsigned long long total_bytes = 0;
    unsigned long long h = 14695981039346656037ull;
    const unsigned long long N = 1000000ull;

    for (unsigned long long t = 0; t < N; t++) {
        size_t len;
        switch (rng_next() % 10) {
        case 0: case 1: case 2: case 3: case 4: case 5:
            len = rng_next() % 128;              /* small, incl. empty */
            break;
        case 6: case 7:
            len = 128 + (rng_next() % 1920);     /* 128..2047 */
            break;
        case 8:
            len = 2048 + (rng_next() % (MAXLEN - 2048)); /* 2048..20015 */
            break;
        default:
            len = 250 + (rng_next() % 11);       /* 250..260, at mod-255 */
            break;
        }
        for (size_t i = 0; i < len; i++)
            buf[i] = (unsigned char)rng_next();
        total_bytes += len;

        unsigned int got = fletcher16(buf, len);
        unsigned int want = ref_fletcher16(buf, len);
        h = fnv1a(h, got);
        if (got != want) {
            if (mismatches < 5)
                printf("MISMATCH len=%zu got=0x%04x want=0x%04x\n",
                       len, got, want);
            mismatches++;
        }
    }

    printf("differential: %llu buffers, %llu bytes total, seed=0xF1E7C416, "
           "mismatches=%llu\n", N, total_bytes, mismatches);
    printf("fnv1a=0x%016llx\n", h);
    return mismatches != 0;
}

static int benchmark(void)
{
    static unsigned char bench[16 * 1024 * 1024]; /* 16 MiB */
    for (size_t i = 0; i < sizeof bench; i++)
        bench[i] = (unsigned char)(rng_next() & 0xff);

    const unsigned long long target = 256ull * 1024 * 1024; /* 256 MiB */
    unsigned long long done = 0;
    unsigned int acc = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (done < target) {
        acc ^= fletcher16(bench, sizeof bench);
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
