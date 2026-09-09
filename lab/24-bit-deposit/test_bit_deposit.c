#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "bit_deposit.h"

/* splitmix64, fixed seed: reproducible random 64-bit stream. */
static uint64_t rng_state = 0x123456789ABCDEF0u;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15u);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9u;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBu;
    return z ^ (z >> 31);
}

/*
 * Naive per-bit references. Extract: bit k of the field must equal
 * bit off + k of the word. Insert: each destination bit off + k is
 * set or cleared from the matching source bit of v. No composite
 * mask identity is used here, so agreement with the implementation
 * proves the shift/mask construction bit by bit.
 */
static uint64_t ref_extract(uint64_t x, unsigned off, unsigned w)
{
    uint64_t r = 0;
    for (unsigned k = 0; k < w; k++)
        if ((x >> (off + k)) & 1u)
            r |= 1ULL << k;
    return r;
}

static uint64_t ref_insert(uint64_t x, unsigned off, unsigned w, uint64_t v)
{
    uint64_t r = x;
    for (unsigned k = 0; k < w; k++) {
        if ((v >> k) & 1u)
            r |= 1ULL << (off + k);
        else
            r &= ~(1ULL << (off + k));
    }
    return r;
}

/* Mask in closed form for the round-trip expectation (w <= 63 here,
 * so the shift is always defined). */
static uint64_t ref_mask(unsigned w)
{
    return (1ULL << w) - 1u;
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

int main(void)
{
    uint64_t mismatches = 0, checked = 0;

    /*
     * Exhaustive differential: every width 1..8, every offset 0..15,
     * all 65536 16-bit inputs, extract and insert each compared
     * against the naive per-bit loop. The inserted value v is a
     * deterministic LCG-style scramble of x so varied bit patterns
     * are deposited. 8 * 16 * 65536 = 8,388,608 cases per primitive.
     */
    for (unsigned w = 1; w <= 8; w++) {
        for (unsigned off = 0; off <= 15; off++) {
            for (uint64_t x = 0; x < 65536u; x++) {
                uint64_t v = ((x * 1103515245u) + 12345u) & 0xFFFFu;
                uint64_t e = bit_extract(x, off, w);
                uint64_t re = ref_extract(x, off, w);
                if (e != re) {
                    mismatches++;
                    if (mismatches < 4)
                        printf("EXTRACT MISMATCH x=%" PRIu64
                               " off=%u w=%u got=%" PRIu64
                               " ref=%" PRIu64 "\n",
                               x, off, w, e, re);
                }
                checked++;
                checksum_add(e);
                uint64_t ins = bit_insert(x, off, w, v);
                uint64_t rins = ref_insert(x, off, w, v);
                if (ins != rins) {
                    mismatches++;
                    if (mismatches < 4)
                        printf("INSERT MISMATCH x=%" PRIu64
                               " off=%u w=%u v=%" PRIu64 " got=%" PRIu64
                               " ref=%" PRIu64 "\n",
                               x, off, w, v, ins, rins);
                }
                checked++;
                checksum_add(ins);
            }
        }
    }
    printf("exhaustive: checked=%" PRIu64 " mismatches=%" PRIu64 "\n",
           checked, mismatches);

    /*
     * Round-trip invariant: depositing the extracted field back into
     * a zero word must reproduce exactly the field bits of x and
     * clear everything else:
     *   bit_insert(0, off, w, bit_extract(x, off, w)) == x & (mask_w << off)
     * All widths 1..63, 1,000,000 fixed-seed values each, the offset
     * drawn from the same fixed-seed stream.
     */
    {
        uint64_t rt_checked = 0, rt_mismatches = 0;
        for (unsigned w = 1; w <= 63; w++) {
            uint64_t m = ref_mask(w);
            for (uint64_t i = 0; i < 1000000u; i++) {
                uint64_t x = splitmix64();
                unsigned off = (unsigned)(splitmix64() % (uint64_t)(65u - w));
                uint64_t back = bit_insert(0, off, w, bit_extract(x, off, w));
                uint64_t expect = x & (m << off);
                if (back != expect) {
                    rt_mismatches++;
                    if (rt_mismatches < 4)
                        printf("ROUNDTRIP MISMATCH x=%016" PRIx64
                               " off=%u w=%u back=%016" PRIx64
                               " expect=%016" PRIx64 "\n",
                               x, off, w, back, expect);
                }
                rt_checked++;
                checksum_add(back);
            }
        }
        checked += rt_checked;
        mismatches += rt_mismatches;
        printf("roundtrip: checked=%" PRIu64 " mismatches=%" PRIu64 "\n",
               rt_checked, rt_mismatches);
    }

    printf("total: checked=%" PRIu64 " mismatches=%" PRIu64 "\n",
           checked, mismatches);
    printf("checksum=%" PRIu64 "\n", fnv1a);

    /*
     * Throughput: 100M timed iterations, one extract and one insert
     * per iteration on PRNG-fed values, XORed into a sink so the
     * loop cannot be optimized away.
     */
    {
        const uint64_t N_TIMED = 100000000u;
        uint64_t sink = 0;
        uint64_t t0 = ns_now();
        for (uint64_t i = 0; i < N_TIMED; i++) {
            uint64_t x = splitmix64();
            sink ^= bit_extract(x, 7, 19);
            sink ^= bit_insert(x, 7, 19, sink);
        }
        uint64_t t1 = ns_now();
        uint64_t ns_total = t1 - t0;
        printf("throughput: iters=%" PRIu64 " ns_total=%" PRIu64
               " ns_per_iter=%.2f sink=%" PRIu64 "\n",
               N_TIMED, ns_total,
               (double)ns_total / (double)N_TIMED, sink);
    }

    if (mismatches == 0) {
        printf("PASS\n");
        return 0;
    }
    printf("FAIL\n");
    return 1;
}
