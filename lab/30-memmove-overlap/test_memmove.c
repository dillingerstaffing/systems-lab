#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "memmove30.h"

#define BUF 256   /* arena */
#define SRC 96    /* src anchor: dest offsets -64..+64 stay in bounds */

/* Fixed-seed xorshift64*: every run fills the arena with identical bytes. */
static uint64_t rng_state = 0x123456789ABCDEF1ull;

static uint64_t next_rand(void)
{
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 0x2545F4914F6CDD1Dull;
}

static void fill(unsigned char *b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        b[i] = (unsigned char)(next_rand() >> 56);
}

/* Broken by design: always copies forward regardless of geometry. Used
 * only to demonstrate the corruption mechanism on a backward overlap. */
static void naive_forward(unsigned char *d, const unsigned char *s, size_t n)
{
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
}

static uint64_t total_cases;
static uint64_t mismatches;
static uint64_t checksum;

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* One differential case: identical content, same geometry; mine vs libc.
 * memcmp compares two distinct arrays, so no overlap UB is possible. */
static void check_case(size_t size, int off)
{
    unsigned char mine[BUF], ref[BUF];

    fill(mine, BUF);
    memcpy(ref, mine, BUF);

    my_memmove(mine + SRC + off, mine + SRC, size);
    memmove(ref + SRC + off, ref + SRC, size);

    total_cases++;
    if (memcmp(mine, ref, BUF) != 0) {
        mismatches++;
        if (mismatches < 10)
            printf("MISMATCH size=%zu off=%d\n", size, off);
    }
    for (size_t i = 0; i < BUF; i++)
        checksum = checksum * 0x100000001B3ull + mine[i] + (uint64_t)i;
}

/* Directed case: backward overlap (dest > src) where naive forward copy
 * corrupts but direction-aware copy must stay byte-exact. */
static int directed_backward(void)
{
    unsigned char mine[32], naive[32];
    unsigned char expect[8] = {'A','B','C','D','E','F','G','H'};

    fill(mine, sizeof(mine));
    memcpy(naive, mine, sizeof(naive));

    memcpy(mine + 8, expect, 8);
    memcpy(naive + 8, expect, 8);

    my_memmove(mine + 12, mine + 8, 8);    /* overlap: dest 4 bytes above src */
    naive_forward(naive + 12, naive + 8, 8);

    int ok = memcmp(mine + 12, expect, 8) == 0;
    int corrupt = memcmp(naive + 12, expect, 8) != 0;

    printf("directed backward: mine_matches=%d naive_corrupts=%d ", ok, corrupt);
    printf("naive_got=");
    for (int i = 0; i < 8; i++)
        printf("%c", naive[12 + i]);
    printf("\n");
    return ok && corrupt;
}

/* Directed case: forward overlap (dest < src), both directions agree. */
static int directed_forward(void)
{
    unsigned char mine[32], ref[32];
    unsigned char expect[8] = {'A','B','C','D','E','F','G','H'};

    fill(mine, sizeof(mine));
    memcpy(ref, mine, sizeof(ref));

    memcpy(mine + 12, expect, 8);
    memcpy(ref + 12, expect, 8);

    my_memmove(mine + 8, mine + 12, 8);    /* overlap: dest 4 bytes below src */
    memmove(ref + 8, ref + 12, 8);

    int ok = memcmp(mine, ref, sizeof(mine)) == 0;
    printf("directed forward: matches_libc=%d\n", ok);
    return ok;
}

/* Throughput: backward overlap, 4 MiB per copy, checksum-folded so the
 * compiler cannot drop the work. Reports MiB/s for both copies. */
static void bench(void)
{
    const size_t SZ = 4 * 1024 * 1024;
    const size_t SHIFT = 4096;
    const int ITERS = 200;
    static unsigned char *b;
    b = (unsigned char *)malloc(SZ + SHIFT);
    if (!b) {
        printf("bench: malloc failed\n");
        return;
    }
    for (size_t i = 0; i < SZ + SHIFT; i++)
        b[i] = (unsigned char)i;

    unsigned char *src = b;
    unsigned char *dst = b + SHIFT;   /* dest > src: backward overlap */

    double t0 = now_ns();
    uint64_t acc = 0;
    for (int i = 0; i < ITERS; i++) {
        my_memmove(dst, src, SZ);
        for (size_t j = 0; j < 16; j++)
            acc += dst[j * 65537];
    }
    double t1 = now_ns();
    double mine_mibs = (double)(SZ * (size_t)ITERS) * 1e9 / (t1 - t0) / (1024.0 * 1024.0);

    t0 = now_ns();
    for (int i = 0; i < ITERS; i++) {
        memmove(dst, src, SZ);
        for (size_t j = 0; j < 16; j++)
            acc += dst[j * 65537];
    }
    t1 = now_ns();
    double libc_mibs = (double)(SZ * (size_t)ITERS) * 1e9 / (t1 - t0) / (1024.0 * 1024.0);

    printf("bench backward_overlap 4MiB x %d: my_memmove=%.1f MiB/s libc_memmove=%.1f MiB/s acc=%llu\n",
           ITERS, mine_mibs, libc_mibs, (unsigned long long)acc);
    free(b);
}

int main(void)
{
    for (size_t size = 0; size <= 64; size++)
        for (int off = -64; off <= 64; off++)
            check_case(size, off);

    printf("total_cases=%llu mismatches=%llu checksum=%llu\n",
           (unsigned long long)total_cases,
           (unsigned long long)mismatches,
           (unsigned long long)checksum);

    int d1 = directed_backward();
    int d2 = directed_forward();
    bench();

    if (mismatches == 0 && d1 && d2) {
        printf("PASS\n");
        return 0;
    }
    printf("FAIL\n");
    return 1;
}
