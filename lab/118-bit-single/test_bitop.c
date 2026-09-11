#include <stdint.h>
#include <stdio.h>

#include "bitop.h"

/* Differential test: bset/bclr/btg/btst against naive per-bit loop
 * references that never use the (1ULL << i) mask identities.
 *
 *  - directed values (68): 0, UINT64_MAX, every single-bit word 1ULL<<k
 *    (k = 0..63), and the two alternating patterns 0xAAAAAAAAAAAAAAAA and
 *    0x5555555555555555, crossed with all 64 positions i = 0..63
 *  - 1,000,000 fixed-seed splitmix64 (word, position) pairs
 *    (seed 0x123456789ABCDEF0, position = rand % 64)
 *
 * Each check compares one of the four operations against its reference.
 * The i = 63 edge is pinned explicitly against known truths. An FNV-1a
 * 64-bit checksum over every result lets builds be cross-checked.
 */

static uint64_t rng_state = 0x123456789ABCDEF0ull;

static uint64_t splitmix64(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static uint64_t fnv1a = 14695981039346656037ull;

static void fnv_mix(uint64_t v)
{
    int b;
    for (b = 0; b < 8; b++) {
        fnv1a ^= (v >> (8 * b)) & 0xFFull;
        fnv1a *= 1099511628211ull;
    }
}

/* Naive references: rebuild the answer one bit at a time. bit_at(x, j)
 * moves bit j to position 0 with a plain right shift (shift counts stay
 * within 0..63); no mask identity is used anywhere here. */
static int bit_at(uint64_t x, int j)
{
    return (int)((x >> j) & 1ULL);
}

static uint64_t ref_set(uint64_t x, int i)
{
    uint64_t r = 0;
    int j;
    for (j = 0; j < 64; j++) {
        int b = (j == i) ? 1 : bit_at(x, j);
        r |= (uint64_t)b << j;
    }
    return r;
}

static uint64_t ref_clr(uint64_t x, int i)
{
    uint64_t r = 0;
    int j;
    for (j = 0; j < 64; j++) {
        int b = (j == i) ? 0 : bit_at(x, j);
        r |= (uint64_t)b << j;
    }
    return r;
}

static uint64_t ref_tg(uint64_t x, int i)
{
    uint64_t r = 0;
    int j;
    for (j = 0; j < 64; j++) {
        int b = (j == i) ? !bit_at(x, j) : bit_at(x, j);
        r |= (uint64_t)b << j;
    }
    return r;
}

static int ref_tst(uint64_t x, int i)
{
    int j;
    for (j = 0; j < 64; j++) {
        if (j == i)
            return bit_at(x, j);
    }
    return -1; /* unreachable: i is always in 0..63 here */
}

static uint64_t mismatches;

static void check_pair(uint64_t x, int i)
{
    uint64_t gs = bset(x, i), rs = ref_set(x, i);
    uint64_t gc = bclr(x, i), rc = ref_clr(x, i);
    uint64_t gt = btg(x, i), rt = ref_tg(x, i);
    int ts = btst(x, i), tr = ref_tst(x, i);

    if (gs != rs) {
        if (mismatches < 8)
            printf("MISMATCH set   x=0x%016llx i=%d got=0x%016llx ref=0x%016llx\n",
                   (unsigned long long)x, i,
                   (unsigned long long)gs, (unsigned long long)rs);
        mismatches++;
    }
    if (gc != rc) {
        if (mismatches < 8)
            printf("MISMATCH clear x=0x%016llx i=%d got=0x%016llx ref=0x%016llx\n",
                   (unsigned long long)x, i,
                   (unsigned long long)gc, (unsigned long long)rc);
        mismatches++;
    }
    if (gt != rt) {
        if (mismatches < 8)
            printf("MISMATCH toggle x=0x%016llx i=%d got=0x%016llx ref=0x%016llx\n",
                   (unsigned long long)x, i,
                   (unsigned long long)gt, (unsigned long long)rt);
        mismatches++;
    }
    if (ts != tr) {
        if (mismatches < 8)
            printf("MISMATCH test  x=0x%016llx i=%d got=%d ref=%d\n",
                   (unsigned long long)x, i, ts, tr);
        mismatches++;
    }
    fnv_mix(gs);
    fnv_mix(gc);
    fnv_mix(gt);
    fnv_mix((uint64_t)ts);
}

int main(void)
{
    static const uint64_t extra[] = {
        0ull,
        0xFFFFFFFFFFFFFFFFull,
        0xAAAAAAAAAAAAAAAAull,
        0x5555555555555555ull,
    };
    uint64_t directed = 0;
    uint64_t n;
    int i;
    size_t k;

    /* Pin the i = 63 edge against known truths. Bit 63 is the MSB. */
    if ((1ULL << 63) != 0x8000000000000000ull) {
        printf("EDGE-FAIL: mask(63) is not the MSB\n");
        return 1;
    }
    if (bset(0ull, 63) != 0x8000000000000000ull ||
        bclr(0xFFFFFFFFFFFFFFFFull, 63) != 0x7FFFFFFFFFFFFFFFull ||
        btg(0x8000000000000000ull, 63) != 0ull ||
        btst(0x8000000000000000ull, 63) != 1 ||
        btst(0x7FFFFFFFFFFFFFFFull, 63) != 0) {
        printf("EDGE-FAIL: i=63 edge pin failed\n");
        return 1;
    }
    printf("edge i=63 pinned: mask=0x8000000000000000\n");

    /* Directed sweep: 68 values crossed with all 64 positions. */
    for (k = 0; k < sizeof extra / sizeof extra[0]; k++)
        for (i = 0; i < 64; i++) {
            check_pair(extra[k], i);
            directed++;
        }
    for (k = 0; k < 64; k++)
        for (i = 0; i < 64; i++) {
            check_pair(1ULL << k, i);
            directed++;
        }
    printf("directed done: %llu (word,position) pairs, %llu checks\n",
           (unsigned long long)directed, (unsigned long long)(directed * 4ull));

    /* 1,000,000 fixed-seed (word, position) pairs. */
    rng_state = 0x123456789ABCDEF0ull;
    for (n = 0; n < 1000000ull; n++) {
        uint64_t w = splitmix64();
        int p = (int)(splitmix64() % 64ull);
        check_pair(w, p);
    }
    printf("random done: 1000000 (word,position) pairs, splitmix64 seed 0x123456789ABCDEF0\n");

    printf("total checks          : %llu\n",
           (unsigned long long)(directed * 4ull + 4000000ull));
    printf("mismatches            : %llu\n", (unsigned long long)mismatches);
    printf("FNV-1a of all results : 0x%016llx\n", (unsigned long long)fnv1a);
    return mismatches == 0 ? 0 : 1;
}
