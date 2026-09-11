#define _POSIX_C_SOURCE 199309L /* clock_gettime for the BENCH build */
/* Differential test for lab/122-decimal-digits.
 *
 * Oracle: strlen of snprintf(buf, "%llu") over
 *   phase 1: directed boundaries, 0, 10^k and 10^k-1 for k = 1..19,
 *            and UINT64_MAX (40 rows, printed);
 *   phase 2: 10,000,000 fixed-seed splitmix64 64-bit values
 *            (seed 0x123456789ABCDEF0);
 *   phase 3: every uint32_t value 0 .. 2^32 - 1 (4,294,967,296 checks).
 * Checksum: FNV-1a over every implementation output byte, so the whole
 * corpus is pinned to one value and re-checkable across build variants.
 */
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#ifdef BENCH
#include "decimal_digits.c"
#else
#include "decimal_digits.h"
#endif

#ifdef BENCH
/* Throughput benchmark: best of 5 runs over 25M splitmix64 values. */
int main(void)
{
    static const uint64_t N = 25000000ULL;
    static const int ROUNDS = 5;
    uint64_t state = 0x123456789ABCDEF0ULL;
    double best = 1e30;
    uint64_t sink = 0;

    for (int r = 0; r < ROUNDS; r++) {
        state = 0x123456789ABCDEF0ULL;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (uint64_t i = 0; i < N; i++) {
            uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            uint64_t x = z ^ (z >> 31);
            sink += digit_count(x);
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double s = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
        double ns = s * 1e9 / (double)N;
        if (ns < best)
            best = ns;
    }
    printf("throughput: %.2f ns/value at -O2 (best of %d, %llu values, sink=%llu)\n",
           best, ROUNDS, (unsigned long long)N, (unsigned long long)sink);
    return 0;
}
#else

/* splitmix64: deterministic stream of test inputs. */
static uint64_t rng_state = 0x123456789ABCDEF0ULL;
static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a 64-bit over the implementation's output bytes. */
static uint64_t fnv = 1469598103934665603ULL;
static void fnv_update(uint8_t b)
{
    fnv ^= b;
    fnv *= 1099511628211ULL;
}

static char obuf[32];

static uint64_t checks = 0, mismatches = 0;

static void check_one(uint64_t x, unsigned expected)
{
    snprintf(obuf, sizeof obuf, "%llu", (unsigned long long)x);
    unsigned oracle = (unsigned)strlen(obuf);
    uint8_t got = digit_count(x);
    checks++;
    if (got != oracle || (expected && got != expected))
        mismatches++;
    fnv_update(got);
}

int main(void)
{
    /* Phase 1: directed boundaries. For k = 1..19, 10^k - 1 has exactly
     * k digits and 10^k has exactly k + 1 digits; 0 has 1 digit;
     * UINT64_MAX has 20 digits. */
    printf("phase1 boundaries (x, expected, got):\n");
    check_one(0, 1);
    printf("  x=%20llu expected=%2u got=%2u\n",
           0ULL, 1u, (unsigned)digit_count(0));
    for (unsigned k = 1; k <= 19; k++) {
        uint64_t lo = 1, hi = 1;
        for (unsigned j = 0; j < k; j++) {
            lo *= 10;
            hi *= 10;
        }
        lo -= 1; /* 10^k - 1 */
        check_one(lo, k);
        printf("  x=%20llu expected=%2u got=%2u\n",
               (unsigned long long)lo, k, (unsigned)digit_count(lo));
        check_one(hi, k + 1);
        printf("  x=%20llu expected=%2u got=%2u\n",
               (unsigned long long)hi, k + 1, (unsigned)digit_count(hi));
    }
    check_one(UINT64_MAX, 20);
    printf("  x=%20llu expected=%2u got=%2u\n",
           (unsigned long long)UINT64_MAX, 20u,
           (unsigned)digit_count(UINT64_MAX));

    /* Phase 2: 10M fixed-seed random 64-bit values. */
    printf("phase2 random: ");
    fflush(stdout);
    for (uint64_t i = 0; i < 10000000ULL; i++)
        check_one(splitmix64(), 0);
    printf("done\n");

    /* Phase 3: every 32-bit value. */
    printf("phase3 full 2^32 sweep: ");
    fflush(stdout);
    for (uint64_t i = 0; i < 0x100000000ULL; i++) {
        check_one((uint32_t)i, 0);
        if ((i & 0x3FFFFFFFULL) == 0) {
            fprintf(stderr, "  phase3 progress: %llu / 4294967296\n",
                    (unsigned long long)i);
            fflush(stderr);
        }
    }
    printf("done\n");

    printf("total checks: %llu\n", (unsigned long long)checks);
    printf("mismatches: %llu\n", (unsigned long long)mismatches);
    printf("checksum: %016llx\n", (unsigned long long)fnv);
    printf("%s\n", mismatches == 0 ? "PASS" : "FAIL");
    return mismatches == 0 ? 0 : 1;
}
#endif
