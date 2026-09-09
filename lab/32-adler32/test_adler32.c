/*
 * test_adler32.c: verifies adler32() three ways.
 *
 * 1. RFC 1950 known-answer vectors (empty, "a", "abc", "message digest",
 *    "abcdefghijklmnopqrstuvwxyz", 0x00..0xFF repeated 100 times).
 * The a..z and seq100 expected values were cross-checked with an
 * independent implementation (Python's zlib.adler32) because the two
 * values originally listed for them did not match; the implementation
 * agreed with the independent check on all six vectors.
 * 2. Differential test against a naive two-loop reference that reduces
 *    modulo 65521 after every single byte, over 1M fixed-seed random
 *    buffers with lengths covering empty, odd sizes, and the 5552-byte
 *    block boundary.
 * 3. Throughput benchmark at the compiled optimization level.
 *
 * Exit code 0 means every check passed; anything else is a failure.
 */
#define _POSIX_C_SOURCE 199309L /* for clock_gettime */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "adler32.h"

/* Deterministic PRNG (splitmix64-style LCG). Seed is fixed and stated. */
static unsigned long long rng_state = 0xAD1E4321ull; /* the fixed seed */
static unsigned int rng_next(void) {
    rng_state = rng_state * 6364136223846793005ull + 1442695040888963407ull;
    return (unsigned int)(rng_state >> 33);
}

/*
 * Naive reference: the recurrence applied exactly as written, one byte at
 * a time, reducing both sums modulo 65521 after every byte. Trivially
 * correct by construction; the differential test checks that the blocked
 * implementation agrees with it on every input.
 */
static unsigned int ref_adler32(const unsigned char *data, size_t len) {
    unsigned int a = 1, b = 0;
    for (size_t i = 0; i < len; i++) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

static int check_vectors(void) {
    struct vec {
        const char *desc;
        const unsigned char *data;
        size_t len;
        unsigned int expect;
    } vecs[6];
    static const unsigned char v_a[] = "a";
    static const unsigned char v_abc[] = "abc";
    static const unsigned char v_msg[] = "message digest";
    static const unsigned char v_alpha[] = "abcdefghijklmnopqrstuvwxyz";
    static unsigned char v_seq[256 * 100];

    for (int r = 0; r < 100; r++)
        for (int i = 0; i < 256; i++)
            v_seq[r * 256 + i] = (unsigned char)i;

    vecs[0] = (struct vec){"empty", NULL, 0, 0x00000001u};
    vecs[1] = (struct vec){"\"a\"", v_a, 1, 0x00620062u};
    vecs[2] = (struct vec){"\"abc\"", v_abc, 3, 0x024d0127u};
    vecs[3] = (struct vec){"\"message digest\"", v_msg, 14, 0x29750586u};
    vecs[4] = (struct vec){"a..z", v_alpha, 26, 0x90860b20u};
    vecs[5] = (struct vec){"0x00..0xFF x100", v_seq, sizeof v_seq, 0x747fd0e0u};

    int fails = 0;
    for (int i = 0; i < 6; i++) {
        unsigned int got = adler32(vecs[i].data, vecs[i].len);
        unsigned int ref = ref_adler32(vecs[i].data, vecs[i].len);
        int ok = (got == vecs[i].expect) && (got == ref);
        printf("vector %-18s expect=0x%08x got=0x%08x %s\n",
               vecs[i].desc, vecs[i].expect, got, ok ? "OK" : "FAIL");
        fails += !ok;
    }
    return fails;
}

/* Largest buffer any length class below can produce. */
#define MAXLEN 20016

static int differential_test(void) {
    static unsigned char buf[MAXLEN];
    unsigned long long mismatches = 0;
    unsigned long long total_bytes = 0;
    const unsigned long long N = 1000000ull;

    for (unsigned long long t = 0; t < N; t++) {
        size_t len;
        switch (rng_next() % 10) {
        case 0: case 1: case 2: case 3: case 4: case 5:
            len = rng_next() % 128;          /* small, incl. empty and odd */
            break;
        case 6: case 7:
            len = 5544 + (rng_next() % 17);  /* 5544..5560: across NMAX */
            break;
        case 8:
            len = 11096 + (rng_next() % 17); /* across 2*NMAX */
            break;
        default:
            len = rng_next() % 20001;        /* 0..20000: multi-block */
            break;
        }
        for (size_t i = 0; i < len; i++)
            buf[i] = (unsigned char)rng_next();
        total_bytes += len;

        unsigned int got = adler32(buf, len);
        unsigned int want = ref_adler32(buf, len);
        if (got != want) {
            if (mismatches < 5)
                printf("MISMATCH len=%zu got=0x%08x want=0x%08x\n",
                       len, got, want);
            mismatches++;
        }
    }

    printf("differential: %llu buffers, %llu bytes total, seed=0xAD1E4321, "
           "mismatches=%llu\n",
           N, total_bytes, mismatches);
    return mismatches != 0;
}

static int benchmark(void) {
    static unsigned char bench[16 * 1024 * 1024]; /* 16 MiB */
    for (size_t i = 0; i < sizeof bench; i++)
        bench[i] = (unsigned char)(rng_next() & 0xff);

    const unsigned long long target = 256ull * 1024 * 1024; /* 256 MiB */
    unsigned long long done = 0;
    unsigned int acc = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while (done < target) {
        acc ^= adler32(bench, sizeof bench);
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

int main(void) {
    int fails = 0;
    fails += check_vectors();
    fails += differential_test();
    fails += benchmark();
    printf(fails ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", fails);
    return fails != 0;
}
