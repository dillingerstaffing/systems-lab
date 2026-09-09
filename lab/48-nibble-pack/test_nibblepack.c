#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "nibblepack.h"

/* ------------------------------------------------------------------
 * Independent reference: builds words from a 16x4 bit truth table and a
 * table of literal place values, using only multiply/add/divide to put
 * bits together. It shares no shift, OR, or mask construction with the
 * implementation under test.
 * ------------------------------------------------------------------ */

/* bit b (0 = least significant) of nibble value v, written as literal 0/1 */
static const int BIT_T[16][4] = {
    {0,0,0,0}, {1,0,0,0}, {0,1,0,0}, {1,1,0,0},
    {0,0,1,0}, {1,0,1,0}, {0,1,1,0}, {1,1,1,0},
    {0,0,0,1}, {1,0,0,1}, {0,1,0,1}, {1,1,0,1},
    {0,0,1,1}, {1,0,1,1}, {0,1,1,1}, {1,1,1,1},
};

/* place value of each of the 64 bit positions, as literal constants */
static const uint64_t PLACE[64] = {
    0x1ull, 0x2ull, 0x4ull, 0x8ull,
    0x10ull, 0x20ull, 0x40ull, 0x80ull,
    0x100ull, 0x200ull, 0x400ull, 0x800ull,
    0x1000ull, 0x2000ull, 0x4000ull, 0x8000ull,
    0x10000ull, 0x20000ull, 0x40000ull, 0x80000ull,
    0x100000ull, 0x200000ull, 0x400000ull, 0x800000ull,
    0x1000000ull, 0x2000000ull, 0x4000000ull, 0x8000000ull,
    0x10000000ull, 0x20000000ull, 0x40000000ull, 0x80000000ull,
    0x100000000ull, 0x200000000ull, 0x400000000ull, 0x800000000ull,
    0x1000000000ull, 0x2000000000ull, 0x4000000000ull, 0x8000000000ull,
    0x10000000000ull, 0x20000000000ull, 0x40000000000ull, 0x80000000000ull,
    0x100000000000ull, 0x200000000000ull, 0x400000000000ull,
    0x800000000000ull,
    0x1000000000000ull, 0x2000000000000ull, 0x4000000000000ull,
    0x8000000000000ull,
    0x10000000000000ull, 0x20000000000000ull, 0x40000000000000ull,
    0x80000000000000ull,
    0x100000000000000ull, 0x200000000000000ull, 0x400000000000000ull,
    0x800000000000000ull,
    0x1000000000000000ull, 0x2000000000000000ull, 0x4000000000000000ull,
    0x8000000000000000ull,
};

/* place values of the four bit positions inside one nibble */
static const unsigned NIB_PLACE[4] = {1u, 2u, 4u, 8u};

/* place values of the four nibble positions inside one 16-bit lane */
static const unsigned LANE_DIV[4] = {1u, 16u, 256u, 4096u};

static uint64_t ref_pack(const uint8_t n[16]) {
    uint64_t w = 0;
    for (int i = 0; i < 16; i++) {
        for (int b = 0; b < 4; b++) {
            w += (uint64_t)BIT_T[n[i]][b] * PLACE[4 * i + b];
        }
    }
    return w;
}

static void ref_unpack(uint64_t w, uint8_t out[16]) {
    for (int i = 0; i < 16; i++) {
        unsigned v = 0;
        for (int b = 0; b < 4; b++) {
            v += (unsigned)((w / PLACE[4 * i + b]) % 2u) * NIB_PLACE[b];
        }
        out[i] = (uint8_t)v;
    }
}

/* splitmix64, fixed seed, used to generate the 1M round-trip words */
static uint64_t rng_state;

static uint64_t rng_next(void) {
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

/* FNV-1a 64-bit over the bytes of every packed output */
static uint64_t fnv = 14695981039346656037ull;

static void fnv_feed(uint64_t w) {
    for (int b = 0; b < 8; b++) {
        fnv ^= (uint8_t)(w >> (8 * b));
        fnv *= 1099511628211ull;
    }
}

static int failures = 0;

static void check_vec(const char *name, int cond) {
    if (cond) {
        printf("vector %-28s: pass\n", name);
    } else {
        printf("vector %-28s: FAIL\n", name);
        failures++;
    }
}

int main(void) {
    /* ---- (a) hand-checked known-answer vectors ---- */
    int nvec = 0, npass = 0;
    uint8_t n[16], out[16];

    /* all-zero nibbles pack to 0 */
    for (int i = 0; i < 16; i++) n[i] = 0;
    {
        uint64_t w = pack_nibbles16(n);
        fnv_feed(w);
        nvec++;
        npass += (w == 0ull);
        check_vec("all-zero -> 0", w == 0ull);
    }

    /* every nibble 0xF fills its 4 bits: all 16 hex digits are F */
    for (int i = 0; i < 16; i++) n[i] = 0xF;
    {
        uint64_t w = pack_nibbles16(n);
        fnv_feed(w);
        nvec++;
        npass += (w == 0xFFFFFFFFFFFFFFFFull);
        check_vec("all-0xF -> 0xFFFFFFFFFFFFFFFF",
                  w == 0xFFFFFFFFFFFFFFFFull);
    }

    /* nibbles 0..15: hex digit at position i (from the least significant
     * digit) equals i, so the word reads FEDCBA9876543210 from the most
     * significant digit down */
    for (int i = 0; i < 16; i++) n[i] = (uint8_t)i;
    {
        uint64_t w = pack_nibbles16(n);
        fnv_feed(w);
        nvec++;
        npass += (w == 0xFEDCBA9876543210ull);
        check_vec("0..15 -> 0xFEDCBA9876543210",
                  w == 0xFEDCBA9876543210ull);
    }

    /* inputs above 0xF are reduced to their low 4 bits:
     * low bits F,0,1,B,F,0,7,8,9,A,B,C,D,E,0,0 at positions 15..0 read
     * 00EDCBA9870FB10F */
    {
        const uint8_t wide[16] = {0xFF, 0x10, 0x21, 0xAB,
                                  0x5F, 0x60, 0x77, 0x88,
                                  0x99, 0xAA, 0xBB, 0xCC,
                                  0xDD, 0xEE, 0x10, 0x20};
        for (int i = 0; i < 16; i++) n[i] = wide[i];
        uint64_t w = pack_nibbles16(n);
        fnv_feed(w);
        nvec++;
        npass += (w == 0x00EDCBA9870FB10Full);
        check_vec("masked inputs -> 0x00EDCBA9870FB10F",
                  w == 0x00EDCBA9870FB10Full);
    }

    /* unpack of the hand-computed 0xFEDCBA9876543210 gives nibbles 0..15 */
    {
        const uint8_t want[16] = {0, 1, 2, 3, 4, 5, 6, 7,
                                  8, 9, 10, 11, 12, 13, 14, 15};
        unpack_nibbles16(0xFEDCBA9876543210ull, out);
        int ok = 1;
        for (int i = 0; i < 16; i++) ok &= (out[i] == want[i]);
        nvec++;
        npass += ok;
        check_vec("unpack(0xFEDCBA9876543210) -> 0..15", ok);
    }

    printf("vectors: %d/%d passed\n", npass, nvec);

    /* ---- (b) exhaustive differential test, 16-bit values x 4 lanes ---- */
    uint64_t pack_checks = 0, unpack_checks = 0, mism = 0;
    uint8_t lane4[4], in16[16];
    for (uint32_t v = 0; v < 65536u; v++) {
        for (int j = 0; j < 4; j++) {
            lane4[j] = (uint8_t)((v / LANE_DIV[j]) % 16u);
        }
        for (int lane = 0; lane < 4; lane++) {
            for (int i = 0; i < 16; i++) in16[i] = 0;
            for (int j = 0; j < 4; j++) in16[4 * lane + j] = lane4[j];
            uint64_t w = pack_nibbles16(in16);
            uint64_t r = ref_pack(in16);
            pack_checks++;
            if (w != r) mism++;
            fnv_feed(w);
            uint8_t o1[16], o2[16];
            unpack_nibbles16(w, o1);
            ref_unpack(r, o2);
            unpack_checks++;
            for (int i = 0; i < 16; i++) {
                if (o1[i] != o2[i]) mism++;
            }
        }
    }
    printf("differential: %llu pack checks, %llu unpack checks, %llu mismatches\n",
           (unsigned long long)pack_checks, (unsigned long long)unpack_checks,
           (unsigned long long)mism);
    if (mism) failures++;

    /* ---- (c) round-trip invariant on 1M fixed-seed random words ---- */
    rng_state = 0x123456789ABCDEF0ull; /* stated fixed seed */
    uint64_t rt_mism = 0;
    for (uint64_t t = 0; t < 1000000ull; t++) {
        uint64_t w = rng_next();
        /* nibble-array round trip: unpack(pack(n)) == n */
        for (int i = 0; i < 16; i++) {
            n[i] = (uint8_t)((w / PLACE[4 * i]) % 16u);
        }
        uint64_t p = pack_nibbles16(n);
        fnv_feed(p);
        unpack_nibbles16(p, out);
        for (int i = 0; i < 16; i++) {
            if (out[i] != n[i]) rt_mism++;
        }
        /* word round trip: pack(unpack(w)) == w */
        unpack_nibbles16(w, out);
        uint64_t p2 = pack_nibbles16(out);
        fnv_feed(p2);
        if (p2 != w) rt_mism++;
    }
    printf("round trip: 1000000 random words, unpack(pack(n))==n and "
           "pack(unpack(w))==w, %llu mismatches\n",
           (unsigned long long)rt_mism);
    if (rt_mism) failures++;

    /* ---- (d) FNV-1a checksum over all packed outputs ---- */
    printf("fnv1a-64 over all packed outputs: 0x%016llx\n",
           (unsigned long long)fnv);

    /* ---- (e) throughput benchmark: pack+unpack pairs ---- */
    {
        uint8_t tab[1024][16];
        rng_state = 0x123456789ABCDEF0ull;
        for (int k = 0; k < 1024; k++) {
            uint64_t w = rng_next();
            for (int i = 0; i < 16; i++) {
                tab[k][i] = (uint8_t)((w / PLACE[4 * i]) % 16u);
            }
        }
        volatile uint64_t sink = 0;
        const long NP = 2000000L;
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        for (long k = 0; k < NP; k++) {
            uint8_t tmp[16];
            uint64_t w = pack_nibbles16(tab[k & 1023]);
            unpack_nibbles16(w, tmp);
            sink += w + tmp[k & 15];
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double secs = (t1.tv_sec - t0.tv_sec)
            + (t1.tv_nsec - t0.tv_nsec) / 1e9;
        printf("benchmark: %ld pack+unpack pairs in %.3f s = %.1f ns/pair "
               "(sink=%llu)\n", NP, secs, secs * 1e9 / NP,
               (unsigned long long)sink);
    }

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
    } else {
        printf("%d FAILURE(S)\n", failures);
    }
    return failures != 0;
}
