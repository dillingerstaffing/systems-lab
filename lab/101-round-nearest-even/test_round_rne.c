#define _POSIX_C_SOURCE 200809L /* for popen/pclose under -std=c11 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "round_rne.h"

static uint64_t splitmix64(uint64_t *s);

#ifndef BENCH

/*
 * Independent oracle: computes the exact comparison 2*frac vs 2^32
 * in unsigned __int128, an arithmetic route that shares no bit
 * identities with the implementation (no round_bit/sticky split,
 * no masking; one multiplication against the full modulus).
 * If 2*frac > 2^32 the value is strictly above the midpoint and
 * rounds up; if <, it is strictly below and rounds down; if equal,
 * it is exactly 1/2 and rounds up only when the integer part is
 * odd (ties to even). All arithmetic here is unsigned, so ip + 1u
 * wraps to 0 for ip = 0xFFFFFFFF, the same wraparound contract.
 */
static uint32_t oracle_rne(uint64_t v)
{
    uint32_t ip = (uint32_t)(v >> 32);
    uint32_t fr = (uint32_t)v;
    unsigned __int128 twice = (unsigned __int128)fr * 2u;
    unsigned __int128 half2 = (unsigned __int128)1 << 32;

    if (twice > half2)
        return ip + 1u;
    if (twice < half2)
        return ip;
    return (ip & 1u) ? ip + 1u : ip;
}

static uint64_t fnv1a_update(uint64_t h, uint32_t x)
{
    for (int i = 0; i < 4; i++) {
        h ^= (uint64_t)((x >> (8 * i)) & 0xFFu);
        h *= 1099511628211ULL;
    }
    return h;
}

/* Branchless claim check: the -O2 object must contain no
 * conditional jump (any mnemonic starting with 'j') and no
 * loop-form branch. Returns 1 when the object is branch-free. */
static int mnemonic_is_branch(const char *tok)
{
    if (tok[0] == 'j')
        return 1;
    if (strncmp(tok, "loop", 4) == 0)
        return 1;
    return 0;
}

static int check_branchless(const char *obj)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "objdump -d --no-show-raw-insn %s", obj);
    FILE *p = popen(cmd, "r");
    if (!p)
        return 0;
    char line[512];
    int bad = 0;
    while (fgets(line, sizeof(line), p)) {
        char *col = strchr(line, ':');
        if (!col)
            continue;
        char *t = col + 1;
        while (*t == ' ' || *t == '\t')
            t++;
        if (*t == 0 || *t == '\n')
            continue;
        char tok[32];
        size_t n = 0;
        while (*t && *t != ' ' && *t != '\t' && *t != '\n' &&
               n + 1 < sizeof(tok))
            tok[n++] = *t++;
        tok[n] = 0;
        if (mnemonic_is_branch(tok)) {
            bad = 1;
            break;
        }
    }
    pclose(p);
    return !bad;
}

#endif /* !BENCH */

static uint64_t splitmix64(uint64_t *s)
{
    uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

#ifndef BENCH

int main(void)
{
    if (!check_branchless("round_rne_O2.o")) {
        printf("disasm check: conditional jump found in round_rne_O2.o FAIL\n");
        return 1;
    }
    printf("disasm check: no conditional jumps in round_rne_O2.o OK\n");

    uint64_t checks = 0, mismatches = 0;
    uint64_t sum = 14695981039346656037ULL; /* FNV-1a 64 offset basis */

#define CHECK(v) do { \
        uint64_t _v = (v); \
        uint32_t got = q32_32_round_even(_v); \
        uint32_t want = oracle_rne(_v); \
        if (got != want) { \
            mismatches++; \
            if (mismatches < 8) \
                printf("MISMATCH v=%016llx got=%u want=%u\n", \
                       (unsigned long long)_v, got, want); \
        } \
        sum = fnv1a_update(sum, got); \
        checks++; \
    } while (0)

    /* Exhaustive: all 65,536 values with only the low 16 fraction
     * bits varying, integer part 0. */
    for (uint32_t i = 0; i < 65536u; i++)
        CHECK((uint64_t)i);

    /* Directed rows: the midpoint under every integer-parity
     * combination, sticky set/not set, carry into the integer
     * part, and the pinned wraparound contract. */
    static const uint64_t rows[] = {
        0x0000000080000000ULL, /* tie, integer even -> 0 */
        0x0000000180000000ULL, /* tie, integer odd  -> 2 */
        0x0000000080000001ULL, /* above half (sticky) -> 1 */
        0x000000017FFFFFFFULL, /* below half -> 1 */
        0x000000007FFFFFFFULL, /* below half -> 0 */
        0x00000000FFFFFFFFULL, /* carry into integer part -> 1 */
        0xFFFFFFFFFFFFFFFFULL, /* wraparound contract -> 0 */
        0xFFFFFFFF80000000ULL, /* tie, integer odd, max int -> wraps to 0 */
        0xFFFFFFFF80000001ULL, /* above half, max int -> wraps to 0 */
        0xFFFFFFFE80000000ULL, /* tie, integer even, max-1 -> 0xFFFFFFFE */
        0x123456789ABCDEF0ULL, /* arbitrary dense bit pattern */
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); i++)
        CHECK(rows[i]);

    /* 1,000,000 fixed-seed splitmix64 random 64-bit values. */
    uint64_t st = 0x123456789ABCDEF0ULL;
    for (uint64_t i = 0; i < 1000000ULL; i++)
        CHECK(splitmix64(&st));

    printf("checks: %llu\n", (unsigned long long)checks);
    printf("mismatches: %llu\n", (unsigned long long)mismatches);
    printf("checksum: %016llx\n", (unsigned long long)sum);
    return mismatches ? 1 : 0;
}

#else /* BENCH */

int main(void)
{
    /* The PRNG fill runs once before timing starts; it is NOT
     * inside the timed loop. */
    static uint64_t buf[1u << 20];
    uint64_t st = 0x123456789ABCDEF0ULL;
    for (size_t i = 0; i < sizeof(buf) / sizeof(buf[0]); i++)
        buf[i] = splitmix64(&st);

    double best = 1e100;
    for (int rep = 0; rep < 5; rep++) {
        struct timespec a, b;
        clock_gettime(CLOCK_MONOTONIC, &a);
        uint64_t sink = 0;
        for (size_t i = 0; i < sizeof(buf) / sizeof(buf[0]); i++)
            sink += q32_32_round_even(buf[i]);
        clock_gettime(CLOCK_MONOTONIC, &b);
        double ns = (double)(b.tv_sec - a.tv_sec) * 1e9 +
                    (double)(b.tv_nsec - a.tv_nsec);
        double per = ns / (double)(sizeof(buf) / sizeof(buf[0]));
        printf("rep %d: %.3f ns/value (sink %llu)\n",
               rep, per, (unsigned long long)sink);
        if (per < best)
            best = per;
    }
    printf("best: %.3f ns/value\n", best);
    return 0;
}

#endif
