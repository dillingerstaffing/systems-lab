#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "select.h"

/*
 * Reference: the ternary operator, defined for sel in {0,1}. The
 * reference exists only to check the mask construction against.
 */
static uint64_t ref_select(uint64_t sel, uint64_t a, uint64_t b)
{
    return sel ? b : a;
}

/* splitmix64: deterministic stream of test inputs. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;
static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 14695981039346656037ULL;
static void fnv(uint64_t v)
{
    fnv1a ^= v;
    fnv1a *= 1099511628211ULL;
}

static unsigned long long mismatches = 0;
static unsigned long long checks = 0;

/* Differential check, valid only for sel in {0,1}. */
static void check(uint64_t sel, uint64_t a, uint64_t b)
{
    uint64_t got = bselect(sel, a, b);
    uint64_t want = ref_select(sel, a, b);
    if (got != want) {
        printf("MISMATCH sel=%llu a=%016llx b=%016llx got=%016llx want=%016llx\n",
               (unsigned long long)sel,
               (unsigned long long)a, (unsigned long long)b,
               (unsigned long long)got, (unsigned long long)want);
        mismatches++;
    }
    fnv(got);
    checks++;
}

int main(void)
{
    /* 1. Directed rows, printed for the record. */
    static const struct { uint64_t sel, a, b; } directed[] = {
        {0, 0, 0},
        {0, 0, UINT64_MAX},
        {0, UINT64_MAX, 0},
        {0, 0xDEADBEEFDEADBEEFULL, 0x1234567812345678ULL},
        {1, 0, 0},
        {1, 0, UINT64_MAX},
        {1, UINT64_MAX, 0},
        {1, 0xDEADBEEFDEADBEEFULL, 0x1234567812345678ULL},
        {1, UINT64_MAX, UINT64_MAX},
    };
    printf("directed rows:\n");
    for (size_t i = 0; i < sizeof(directed) / sizeof(directed[0]); i++) {
        uint64_t sel = directed[i].sel, a = directed[i].a, b = directed[i].b;
        uint64_t got = bselect(sel, a, b);
        uint64_t want = ref_select(sel, a, b);
        printf("  sel=%llu a=%016llx b=%016llx bselect=%016llx ref=%016llx %s\n",
               (unsigned long long)sel,
               (unsigned long long)a, (unsigned long long)b,
               (unsigned long long)got, (unsigned long long)want,
               got == want ? "ok" : "FAIL");
        check(sel, a, b);
    }

    /* 2. Out-of-contract rows: sel outside {0,1} is outside the
       contract, so these are printed but never differential-checked
       and never fed to the checksum. The raw arithmetic value is
       shown so the boundary is explicit. */
    static const struct { uint64_t sel, a, b; } outside[] = {
        {2, 0, UINT64_MAX},
        {3, 0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL},
        {UINT64_MAX, 0, UINT64_MAX},
        {0x8000000000000000ULL, UINT64_MAX, 0},
    };
    printf("out-of-contract rows (sel not in {0,1}, shown, not checked):\n");
    for (size_t i = 0; i < sizeof(outside) / sizeof(outside[0]); i++) {
        uint64_t sel = outside[i].sel, a = outside[i].a, b = outside[i].b;
        uint64_t mask = (uint64_t)(-(uint64_t)sel);
        uint64_t got = bselect(sel, a, b);
        printf("  sel=%llu mask=%016llx a=%016llx b=%016llx bselect=%016llx\n",
               (unsigned long long)sel, (unsigned long long)mask,
               (unsigned long long)a, (unsigned long long)b,
               (unsigned long long)got);
    }

    /* 3. Exhaustive 16-bit (sel, a, b) triples: sel in {0,1}, a and b
       over [0, 65535]. 2 * 65536 * 65536 = 8,589,934,592 triples. */
    for (uint64_t sel = 0; sel <= 1; sel++)
        for (uint32_t a = 0; a <= 0xFFFF; a++)
            for (uint32_t b = 0; b <= 0xFFFF; b++)
                check(sel, (uint64_t)a, (uint64_t)b);

    /* 4. 10,000,000 fixed-seed splitmix64 random 64-bit triples. */
    rng_state = 0x123456789ABCDEF0ULL;
    for (int i = 0; i < 10000000; i++) {
        uint64_t sel = splitmix64() & 1;
        uint64_t a = splitmix64();
        uint64_t b = splitmix64();
        check(sel, a, b);
    }

    printf("checks=%llu mismatches=%llu fnv1a=%016llx\n",
           checks, mismatches, (unsigned long long)fnv1a);

#ifdef BENCH
    /*
     * Throughput at -O2, best of 5. 1M triples are generated once
     * before timing; the timed region is 25 passes over that array,
     * XOR-ing each bselect result into a volatile sink. What is
     * measured is bselect plus loop and memory traffic, not the PRNG.
     */
    struct triple { uint64_t sel, a, b; };
    struct triple *vals = malloc(1000000 * sizeof *vals);
    if (!vals) {
        printf("BENCH malloc failed\n");
        return 1;
    }
    rng_state = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < 1000000; i++) {
        vals[i].sel = splitmix64() & 1;
        vals[i].a = splitmix64();
        vals[i].b = splitmix64();
    }

    double best = 1e30;
    for (int rep = 0; rep < 5; rep++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        volatile uint64_t sink = 0;
        for (int pass = 0; pass < 25; pass++)
            for (int i = 0; i < 1000000; i++)
                sink ^= bselect(vals[i].sel, vals[i].a, vals[i].b);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
                    (t1.tv_nsec - t0.tv_nsec);
        if (ns < best)
            best = ns;
        (void)sink;
    }
    free(vals);
    printf("bench: %.2f ns/value (%.1f Mvalues/s over 25M timed values, best of 5)\n",
           best / 25000000.0, 25000000.0 / (best / 1e3));
#endif

    return mismatches == 0 ? 0 : 1;
}
