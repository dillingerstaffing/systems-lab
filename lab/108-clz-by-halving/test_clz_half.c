#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "clz_half.h"

static uint64_t rng_state;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

#ifndef BENCH
/* FNV-1a 64-bit over the result stream. */
static uint64_t fnv = 14695981039346656037ULL;

static void fnv_put(uint64_t v)
{
    fnv ^= v;
    fnv *= 1099511628211ULL;
}

/*
 * Disassemble the -O2 object file of the implementation and fail
 * if any clz-class instruction (bsr, bsf, lzcnt, tzcnt) is present.
 * The implementation must be built from shift/compare identities
 * only; the compiler must not have folded it into a bit-scan
 * instruction behind our back.
 */
static int first_token_is_clz_class(const char *line)
{
    const char *p = strchr(line, ':');
    char tok[16];
    size_t n = 0;

    if (!p)
        return 0;
    p++;
    while (*p == ' ' || *p == '\t')
        p++;
    while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
           (*p >= '0' && *p <= '9')) {
        if (n + 1 < sizeof(tok))
            tok[n++] = (char)(*p | 0x20);
        p++;
    }
    tok[n] = '\0';
    return strcmp(tok, "bsr") == 0 || strcmp(tok, "bsf") == 0 ||
           strcmp(tok, "lzcnt") == 0 || strcmp(tok, "tzcnt") == 0;
}

static int check_no_clz_class_insn(void)
{
    FILE *p = popen("objdump -d --no-show-raw-insn clz_half_O2.o", "r");
    char line[512];
    int found = 0;

    if (!p) {
        printf("disasm check: popen(objdump) failed\n");
        return 1;
    }
    while (fgets(line, sizeof(line), p)) {
        if (first_token_is_clz_class(line)) {
            printf("disasm check: clz-class instruction found: %s", line);
            found = 1;
        }
    }
    if (pclose(p) != 0) {
        printf("disasm check: objdump exited nonzero\n");
        return 1;
    }
    if (!found)
        printf("disasm check: no bsr/bsf/lzcnt/tzcnt in clz_half_O2.o OK\n");
    return found;
}
#endif

#ifdef BENCH

/* Throughput measurement only. Not part of correctness testing. */

#define NBUF (1u << 20)
static uint64_t buf[NBUF];

static double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void)
{
    double best = 0.0;
    uint64_t sink = 0;
    int rep, i;

    rng_state = 0x123456789ABCDEF0ULL;
    for (i = 0; i < (int)NBUF; i++)
        buf[i] = splitmix64();

    /* Warm up. */
    for (i = 0; i < (int)NBUF; i++)
        sink ^= clz64_halving(buf[i]) + (uint64_t)i;

    for (rep = 0; rep < 5; rep++) {
        double t0 = now_ns();
        for (i = 0; i < (int)NBUF; i++)
            sink ^= clz64_halving(buf[i]) + (uint64_t)i;
        {
            double dt = now_ns() - t0;
            double ns = dt / (double)NBUF;
            printf("rep %d: %.3f ns/value (sink %llu)\n",
                   rep, ns, (unsigned long long)sink);
            if (rep == 0 || ns < best)
                best = ns;
        }
    }
    printf("best: %.3f ns/value\n", best);
    return 0;
}

#else

/* Differential test: clz64_halving against __builtin_clzll. */

int main(void)
{
    uint64_t checks = 0;
    uint64_t mismatches = 0;
    uint64_t printed = 0;
    uint64_t v;
    uint32_t i;

    /* The -O2 implementation must use no clz-class instruction. */
    if (check_no_clz_class_insn())
        mismatches++;

    /* All 65536 16-bit inputs, exhaustive. */
    for (i = 0; i < 65536u; i++) {
        uint64_t got = clz64_halving(i);
        uint64_t want = (i == 0) ? 64 : (uint64_t)__builtin_clzll(i);
        fnv_put(got);
        checks++;
        if (got != want) {
            mismatches++;
            if (printed < 8) {
                printf("MISMATCH v=%llu got=%llu want=%llu\n",
                       (unsigned long long)i,
                       (unsigned long long)got,
                       (unsigned long long)want);
                printed++;
            }
        }
    }

    /* x = 0 pinned to the 64 contract, tested separately. */
    {
        uint64_t got = clz64_halving(0);
        fnv_put(got);
        checks++;
        if (got != 64) {
            mismatches++;
            printf("MISMATCH v=0 got=%llu want=64\n",
                   (unsigned long long)got);
        } else {
            printf("x=0 contract: clz64_halving(0) = 64 OK\n");
        }
    }

    /* 10,000,000 fixed-seed 64-bit values. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (i = 0; i < 10000000u; i++) {
        uint64_t got;
        uint64_t want;
        v = splitmix64();
        got = clz64_halving(v);
        want = (v == 0) ? 64 : (uint64_t)__builtin_clzll(v);
        fnv_put(got);
        checks++;
        if (got != want) {
            mismatches++;
            if (printed < 8) {
                printf("MISMATCH v=%llu got=%llu want=%llu\n",
                       (unsigned long long)v,
                       (unsigned long long)got,
                       (unsigned long long)want);
                printed++;
            }
        }
    }

    printf("checks: %llu\n", (unsigned long long)checks);
    printf("mismatches: %llu\n", (unsigned long long)mismatches);
    printf("checksum: %016llx\n", (unsigned long long)fnv);
    return mismatches ? 1 : 0;
}

#endif
