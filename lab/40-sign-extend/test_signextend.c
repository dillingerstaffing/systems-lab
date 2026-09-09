#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "signextend.h"

/*
 * Independent reference for differential verification.
 *
 * It never uses the shift identity under test. Instead it truncates
 * the input to w bits, tests bit (w-1) directly, and either leaves
 * the truncated value alone or ORs in a 1-bit fill of every position
 * at or above w. Both branches are computed with plain bit tests and
 * masks, a separate construction from sign_extend64.
 */
static int64_t sign_extend_ref(uint64_t x, int w)
{
    uint64_t mask = (w == 64) ? ~0ULL : ((1ULL << (unsigned)w) - 1ULL);
    uint64_t xw = x & mask;
    if (w != 64 && (xw & (1ULL << (unsigned)(w - 1))) != 0)
        return (int64_t)(xw | (~0ULL << (unsigned)w));
    return (int64_t)xw;
}

/* FNV-1a 64-bit, folded over every result byte. */
static uint64_t fnv1a_fold(uint64_t h, uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        h ^= (v >> (unsigned)(i * 8)) & 0xFFULL;
        h *= 0x100000001B3ULL;
    }
    return h;
}

static volatile uint64_t sink;

int main(void)
{
    uint64_t checks = 0;
    uint64_t mismatches = 0;
    uint64_t hash = 0xCBF29CE484222325ULL;

    /* Widths 1..63 crossed with every 16-bit input. */
    for (int w = 1; w <= 63; w++) {
        for (uint64_t x = 0; x < 65536ULL; x++) {
            int64_t got = sign_extend64(x, w);
            int64_t want = sign_extend_ref(x, w);
            checks++;
            if (got != want) {
                mismatches++;
                if (mismatches < 8)
                    printf("MISMATCH w=%d x=0x%llx got=%lld want=%lld\n",
                           w, (unsigned long long)x,
                           (long long)got, (long long)want);
            }
            hash = fnv1a_fold(hash, (uint64_t)got);
        }
    }

    /* Width 64 is the identity map; same 16-bit sweep. */
    for (uint64_t x = 0; x < 65536ULL; x++) {
        int64_t got = sign_extend64(x, 64);
        int64_t want = sign_extend_ref(x, 64);
        checks++;
        if (got != want) {
            mismatches++;
            printf("MISMATCH w=64 x=0x%llx got=%lld want=%lld\n",
                   (unsigned long long)x, (long long)got, (long long)want);
        }
        hash = fnv1a_fold(hash, (uint64_t)got);
    }

    /* Directed edges. */
    struct { uint64_t x; int w; } edge[] = {
        { 0, 1 }, { 1, 1 },                                     /* w = 1 */
        { 0x3FFFFFFFFFFFFFFFULL, 63 },                          /* w = 63, high bit clear */
        { 0x7FFFFFFFFFFFFFFFULL, 63 },                          /* w = 63, high bit set   */
        { 0xFFFFFFFFFFFFFFFFULL, 63 },                          /* w = 63, all bits set    */
        { 0x8000000000000000ULL, 64 },                          /* w = 64, INT64_MIN       */
        { 0xFFFFFFFFFFFFFFFFULL, 64 },                          /* w = 64, all bits set    */
    };
    for (unsigned i = 0; i < sizeof(edge) / sizeof(edge[0]); i++) {
        int64_t got = sign_extend64(edge[i].x, edge[i].w);
        int64_t want = sign_extend_ref(edge[i].x, edge[i].w);
        checks++;
        if (got != want) {
            mismatches++;
            printf("EDGE MISMATCH w=%d x=0x%llx got=%lld want=%lld\n",
                   edge[i].w, (unsigned long long)edge[i].x,
                   (long long)got, (long long)want);
        }
        hash = fnv1a_fold(hash, (uint64_t)got);
    }

    printf("checks=%llu mismatches=%llu checksum=0x%016llx\n",
           (unsigned long long)checks, (unsigned long long)mismatches,
           (unsigned long long)hash);

    /* Throughput: timed loop at this build's optimization level. */
    const uint64_t iters = 20000000ULL;
    uint64_t acc = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (uint64_t i = 0; i < iters; i++)
        acc += (uint64_t)sign_extend64(i * 0x9E3779B97F4A7C15ULL,
                                       1 + (int)(i % 64));
    clock_gettime(CLOCK_MONOTONIC, &t1);
    sink = acc;
    uint64_t ns = (uint64_t)(t1.tv_sec - t0.tv_sec) * 1000000000ULL +
                  (uint64_t)(t1.tv_nsec - t0.tv_nsec);
    printf("throughput: %llu values in %llu ns = %.2f ns/value (sink=0x%llx)\n",
           (unsigned long long)iters, (unsigned long long)ns,
           (double)ns / (double)iters, (unsigned long long)sink);

    return mismatches == 0 ? 0 : 1;
}
