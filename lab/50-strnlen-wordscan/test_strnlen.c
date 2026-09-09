#define _POSIX_C_SOURCE 200809L

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

#include "strnlen.h"

/* splitmix64: deterministic RNG for the differential test. */
static uint64_t rng_state = 0x9E3779B97F4A7C15ULL; /* fixed seed */
static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a 64 over every (input, result) pair, so all builds must agree. */
static uint64_t fnv = 0xCBF29CE484222325ULL;
static void fnv_feed_size(size_t v)
{
    for (size_t i = 0; i < sizeof v; i++) {
        fnv ^= (uint64_t)((v >> (8 * i)) & 0xFF);
        fnv *= 0x100000001B3ULL;
    }
}

static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/* Scratch buffer for hand-checked vectors: string at offset m, long enough
 * for the longest vector (len 100 at m up to 7, plus the terminator). */
static char vbuf[512];

static void build_str(size_t m, size_t len, long iz)
{
    for (size_t i = 0; i < len; i++) {
        vbuf[m + i] = (char)(1 + (rng_next() % 255)); /* never zero */
    }
    if (iz >= 0 && (size_t)iz < len) {
        vbuf[m + iz] = 0;
    }
    vbuf[m + len] = 0;
}

static int check_vector(size_t m, size_t len, long iz, size_t max,
                        size_t expect, const char *name)
{
    build_str(m, len, iz);
    size_t got = my_strnlen(vbuf + m, max);
    fnv_feed_size(len);
    fnv_feed_size(m);
    fnv_feed_size(max);
    fnv_feed_size(got);
    if (got != expect) {
        printf("vector %s: len=%zu m=%zu iz=%ld max=%zu -> %zu, "
               "expected %zu: FAIL\n",
               name, len, m, iz, max, got, expect);
        return 1;
    }
    printf("vector %s: len=%zu m=%zu iz=%ld max=%zu -> %zu: pass\n",
           name, len, m, iz, max, got);
    return 0;
}

int main(void)
{
    int fails = 0;
    uint64_t checks = 0;
    uint64_t mismatches = 0;

    /* Hand-checked vectors. */
    fails += check_vector(0, 0, -1, 10, 0, "empty string");
    fails += check_vector(3, 3, -1, 10, 3, "abc at m=3");
    fails += check_vector(3, 3, -1, 2, 2, "abc truncated by max=2");
    fails += check_vector(3, 3, -1, 3, 3, "abc max=3 ends at NUL");
    fails += check_vector(5, 3, -1, 0, 0, "max=0 touches nothing");
    fails += check_vector(2, 3, 1, 10, 1, "interior zero at 1");
    fails += check_vector(1, 7, -1, 100, 7, "len 7, head and tail only");
    fails += check_vector(4, 8, 0, 16, 0, "one word, zero at byte 0");
    fails += check_vector(4, 8, 3, 16, 3, "one word, zero at byte 3");
    fails += check_vector(4, 8, 7, 16, 7, "one word, zero at byte 7");
    fails += check_vector(6, 16, -1, 100, 16, "len 16, NUL at end");
    fails += check_vector(6, 16, -1, 15, 15, "len 16, max=15 no NUL in range");
    fails += check_vector(0, 100, -1, 50, 50, "len 100, max=50 no NUL in range");
    fails += check_vector(7, 1, -1, 1, 1, "single byte string");
    fails += check_vector(7, 1, -1, 10, 1, "single byte, max past NUL");
    printf("vectors done\n");

    /* Differential test vs libc strnlen: lengths 0..256, every
     * misalignment 0..7, two content variants (terminator only, and an
     * interior zero at len/2), five max values each. */
    for (size_t len = 0; len <= 256; len++) {
        for (size_t m = 0; m < 8; m++) {
            char *buf = (char *)malloc(512 + 8);
            if (buf == NULL) {
                printf("differential: malloc failed\n");
                return 1;
            }
            if (((uintptr_t)buf & 15) != 0) {
                printf("differential: malloc not 16-aligned, "
                       "misalignment sweep invalid\n");
                return 1;
            }
            long izs[2] = { -1, (len >= 2) ? (long)(len / 2) : -1 };
            for (int vi = 0; vi < 2; vi++) {
                long iz = izs[vi];
                if (vi == 1 && iz < 0) {
                    continue;
                }
                for (size_t i = 0; i < len; i++) {
                    buf[m + i] = (char)(1 + (rng_next() % 255));
                }
                if (iz >= 0) {
                    buf[m + iz] = 0;
                }
                buf[m + len] = 0;
                size_t maxs[5] = { 0, len, len + 1, len / 2,
                                   len > 0 ? len - 1 : 0 };
                for (int mi = 0; mi < 5; mi++) {
                    size_t maxv = maxs[mi];
                    size_t got = my_strnlen(buf + m, maxv);
                    size_t want = strnlen(buf + m, maxv);
                    fnv_feed_size(len);
                    fnv_feed_size(m);
                    fnv_feed_size(maxv);
                    fnv_feed_size(got);
                    checks++;
                    if (got != want) {
                        if (mismatches < 5) {
                            printf("mismatch: len=%zu m=%zu iz=%ld "
                                   "max=%zu: mine=%zu libc=%zu\n",
                                   len, m, iz, maxv, got, want);
                        }
                        mismatches++;
                    }
                }
            }
            free(buf);
        }
    }
    printf("differential: %llu checks, %llu mismatches\n",
           (unsigned long long)checks, (unsigned long long)mismatches);
    fails += (mismatches != 0);

    /* Guard-page over-read proof: two pages mapped, the second PROT_NONE.
     * Every scan must stop at or before max and never touch the guard.
     * Any over-read faults (ASan also reports it); a clean exit with the
     * right answers proves the bound is respected. */
    {
        long ps = sysconf(_SC_PAGESIZE);
        if (ps <= 0) {
            printf("guard: sysconf failed\n");
            return 1;
        }
        size_t pagesz = (size_t)ps;
        unsigned char *gp =
            (unsigned char *)mmap(NULL, 2 * pagesz, PROT_READ | PROT_WRITE,
                                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (gp == MAP_FAILED) {
            printf("guard: mmap failed\n");
            return 1;
        }
        if (mprotect(gp + pagesz, pagesz, PROT_NONE) != 0) {
            printf("guard: mprotect failed\n");
            return 1;
        }
        int gfail = 0;

        /* Case A: NUL at the last readable byte; scan must stop there. */
        for (size_t off = 0; off < 16; off++) {
            unsigned char *s = gp + pagesz - 120 - off;
            memset(s, 'A', 120);
            s[119] = 0;
            size_t got = my_strnlen((const char *)s, 200);
            fnv_feed_size(off);
            fnv_feed_size(got);
            if (got != 119) {
                printf("guard A off=%zu: got %zu, expected 119: FAIL\n",
                       off, got);
                gfail = 1;
            }
        }

        /* Case B: no NUL in range, max ends exactly at the guard edge.
         * Every head/tail residue 0..15 is covered; a word-at-a-time
         * reader that over-reads would fault here. */
        for (size_t off = 0; off < 16; off++) {
            unsigned char *s = gp + pagesz - 120 - off;
            memset(s, 'B', 120);
            size_t got = my_strnlen((const char *)s, 120);
            fnv_feed_size(off);
            fnv_feed_size(got);
            if (got != 120) {
                printf("guard B off=%zu: got %zu, expected 120: FAIL\n",
                       off, got);
                gfail = 1;
            }
        }

        /* Case C: max=0 with s inside the guard page itself: must return
         * 0 without touching memory. */
        {
            size_t got = my_strnlen((const char *)(gp + pagesz), 0);
            fnv_feed_size(got);
            if (got != 0) {
                printf("guard C: got %zu, expected 0: FAIL\n", got);
                gfail = 1;
            }
        }

        if (munmap(gp, 2 * pagesz) != 0) {
            printf("guard: munmap failed\n");
            return 1;
        }
        if (gfail) {
            fails += 1;
        } else {
            printf("guard: 33 cases, 0 faults, 0 wrong answers: pass\n");
        }
    }

    printf("fnv1a-64 over all results: 0x%llx\n",
           (unsigned long long)fnv);

    /* Throughput: 200-byte string, offsets 0..7 varying each call so the
     * compiler cannot fold the result; the sink defeats dead-code
     * elimination. This is a ceiling on the true per-call cost. */
    {
        static char tbuf[264];
        for (int i = 0; i < 200; i++) {
            tbuf[i] = (char)('a' + (i % 26));
        }
        tbuf[200] = 0;
        const uint64_t N = 20000000ULL;
        uint64_t sink = 0;
        uint64_t t0 = now_ns();
        for (uint64_t i = 0; i < N; i++) {
            sink += my_strnlen(tbuf + (i % 8), 256);
        }
        uint64_t t1 = now_ns();
        double secs = (double)(t1 - t0) / 1e9;
        printf("benchmark: %llu calls in %.3f s = %.1f ns/value "
               "(sink=%llu)\n",
               (unsigned long long)N, secs,
               (double)(t1 - t0) / (double)N,
               (unsigned long long)sink);
    }

    if (fails == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    printf("%d FAILURES\n", fails);
    return 1;
}
