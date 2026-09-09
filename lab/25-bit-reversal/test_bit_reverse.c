#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bit_reverse.h"

/* splitmix64, fixed seed: reproducible random 64-bit stream. */
static uint64_t rng_state = 0x123456789ABCDEF0u;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15u);
    z = (z ^ (z >> 30u)) * 0xBF58476D1CE4E5B9u;
    z = (z ^ (z >> 27u)) * 0x94D049BB133111EBu;
    return z ^ (z >> 31u);
}

/* Reference: the definition itself. Bit k of the input goes to
 * position 63 - k of the output, moved one bit at a time. */
static uint64_t ref_bit_reverse(uint64_t x)
{
    uint64_t r = 0u;
    for (int k = 0; k < 64; k++)
        r |= ((x >> k) & 1u) << (63 - k);
    return r;
}

/* FNV-1a over the raw 8 bytes of each result. */
static uint64_t fnv1a = 14695981039346656037u;

static void checksum_add(uint64_t v)
{
    for (int i = 0; i < 8; i++) {
        fnv1a ^= (uint8_t)(v >> (8 * i));
        fnv1a *= 1099511628211u;
    }
}

static uint64_t ns_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
}

/* One case: differential against the naive reference, plus the
 * involution invariant reverse(reverse(x)) == x. */
static void check(uint64_t x, uint64_t *checked, uint64_t *mismatches,
                  uint64_t *inv_mismatches)
{
    uint64_t got = bit_reverse(x);
    if (got != ref_bit_reverse(x)) {
        (*mismatches)++;
        if (*mismatches < 4)
            printf("MISMATCH x=%" PRIu64 " got=%" PRIu64 " ref=%" PRIu64 "\n",
                   x, got, ref_bit_reverse(x));
    }
    if (bit_reverse(got) != x) {
        (*inv_mismatches)++;
        if (*inv_mismatches < 4)
            printf("INVOLUTION-MISMATCH x=%" PRIu64 "\n", x);
    }
    (*checked)++;
    checksum_add(got);
}

int main(void)
{
    uint64_t mismatches = 0, inv_mismatches = 0, checked = 0;

    /* Explicit boundaries: 0, all-ones, alternating patterns, and
     * every single set bit 2^k for k = 0..63. */
    {
        static const uint64_t vals[] = {
            0u, UINT64_MAX, 0x5555555555555555u, 0xAAAAAAAAAAAAAAAAu
        };
        for (size_t i = 0; i < sizeof vals / sizeof vals[0]; i++)
            check(vals[i], &checked, &mismatches, &inv_mismatches);
        for (int k = 0; k < 64; k++)
            check((uint64_t)1 << k, &checked, &mismatches, &inv_mismatches);
    }

    /* Exhaustive: every 16-bit value embedded at four 16-bit lanes of
     * the 64-bit word, differential against the reference with the
     * involution invariant on every case. */
    for (uint64_t v = 0; v < 65536u; v++) {
        check(v, &checked, &mismatches, &inv_mismatches);
        check(v << 16u, &checked, &mismatches, &inv_mismatches);
        check(v << 32u, &checked, &mismatches, &inv_mismatches);
        check(v << 48u, &checked, &mismatches, &inv_mismatches);
    }

    /* 10M fixed-seed random 64-bit values, same two checks. */
    const uint64_t N_RANDOM = 10000000u;
    for (uint64_t i = 0; i < N_RANDOM; i++)
        check(splitmix64(), &checked, &mismatches, &inv_mismatches);

    printf("differential+involution: checked=%" PRIu64 " mismatches=%" PRIu64
           " involution_mismatches=%" PRIu64 "\n",
           checked, mismatches, inv_mismatches);
    printf("checksum=%" PRIu64 "\n", fnv1a);

    /* Throughput: 100M timed values, PRNG-fed, XORed into a sink so
     * the loop cannot be optimized away. */
    {
        const uint64_t N_TIMED = 100000000u;
        uint64_t sink = 0;
        uint64_t t0 = ns_now();
        for (uint64_t i = 0; i < N_TIMED; i++)
            sink ^= bit_reverse(splitmix64());
        uint64_t t1 = ns_now();
        uint64_t ns_total = t1 - t0;
        printf("throughput: values=%" PRIu64 " ns_total=%" PRIu64
               " ns_per_value=%.2f sink=%" PRIu64 "\n",
               N_TIMED, ns_total,
               (double)ns_total / (double)N_TIMED, sink);
    }

    if (mismatches == 0 && inv_mismatches == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("FAIL\n");
    return 1;
}
