#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "sat_neg.h"

static uint64_t rng_state;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* Reference oracle: the definition itself, as a ternary. */
#ifndef BENCH
static int64_t neg_sat_ref(int64_t x)
{
    return x == INT64_MIN ? INT64_MAX : -x;
}
#endif

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
 * if any conditional jump is present. The implementation is built
 * from sign-mask arithmetic only; the compiler must not have
 * introduced a branch behind our back. Tokens starting with 'j'
 * are jumps on x86; 'jmp' is the unconditional one and is allowed.
 */
static int first_token_is_cond_jump(const char *line)
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
    return tok[0] == 'j' && strcmp(tok, "jmp") != 0;
}

static int check_no_cond_jump(void)
{
    FILE *p = popen("objdump -d --no-show-raw-insn sat_neg_O2.o", "r");
    char line[512];
    int found = 0;

    if (!p) {
        printf("disasm check: popen(objdump) failed\n");
        return 1;
    }
    while (fgets(line, sizeof(line), p)) {
        if (first_token_is_cond_jump(line)) {
            printf("disasm check: conditional jump found: %s", line);
            found = 1;
        }
    }
    if (pclose(p) != 0) {
        printf("disasm check: objdump exited nonzero\n");
        return 1;
    }
    if (!found)
        printf("disasm check: no conditional jump in sat_neg_O2.o OK\n");
    return found;
}
#endif

#ifdef BENCH

/* Throughput measurement only. Not part of correctness testing. */

#define NBUF (1u << 20)
static int64_t buf[NBUF];

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
        buf[i] = (int64_t)splitmix64();

    /* Warm up. */
    for (i = 0; i < (int)NBUF; i++)
        sink ^= (uint64_t)neg_sat64(buf[i]) + (uint64_t)i;

    for (rep = 0; rep < 5; rep++) {
        double t0 = now_ns();
        for (i = 0; i < (int)NBUF; i++)
            sink ^= (uint64_t)neg_sat64(buf[i]) + (uint64_t)i;
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

/* Differential test: neg_sat64 against the ternary reference. */

static void check_one(int64_t v, uint64_t *checks, uint64_t *mismatches,
                      uint64_t *printed)
{
    int64_t got = neg_sat64(v);
    int64_t want = neg_sat_ref(v);

    fnv_put((uint64_t)got);
    (*checks)++;
    if (got != want) {
        (*mismatches)++;
        if (*printed < 8) {
            printf("MISMATCH v=%lld got=%lld want=%lld\n",
                   (long long)v, (long long)got, (long long)want);
            (*printed)++;
        }
    }
}

int main(void)
{
    uint64_t checks = 0;
    uint64_t mismatches = 0;
    uint64_t printed = 0;
    uint32_t i;

    /* The -O2 implementation must contain no conditional jump. */
    if (check_no_cond_jump())
        mismatches++;

    /* Dedicated INT64_MIN row: the saturation edge. */
    {
        int64_t got = neg_sat64(INT64_MIN);
        fnv_put((uint64_t)got);
        checks++;
        if (got != INT64_MAX) {
            mismatches++;
            printf("MISMATCH v=INT64_MIN got=%lld want=%lld\n",
                   (long long)got, (long long)INT64_MAX);
        } else {
            printf("INT64_MIN edge: neg_sat64(INT64_MIN) = INT64_MAX OK\n");
        }
    }

    /* All 65536 16-bit inputs, exhaustive. */
    for (i = 0; i < 65536u; i++)
        check_one((int64_t)(int16_t)i, &checks, &mismatches, &printed);

    /* 10,000,000 fixed-seed 64-bit values. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (i = 0; i < 10000000u; i++)
        check_one((int64_t)splitmix64(), &checks, &mismatches, &printed);

    printf("checks: %llu\n", (unsigned long long)checks);
    printf("mismatches: %llu\n", (unsigned long long)mismatches);
    printf("checksum: %016llx\n", (unsigned long long)fnv);
    return mismatches ? 1 : 0;
}

#endif
