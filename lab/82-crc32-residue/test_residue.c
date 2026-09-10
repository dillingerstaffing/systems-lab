/*
 * lab/82: CRC-32 residue invariant.
 *
 * crc32_*() is the lab/04 implementation, compiled in as-is and used as
 * the oracle. This module only orchestrates it: for each message M it
 * computes c = crc32(M), appends the four bytes of c in little-endian
 * wire order, and checks crc32(M || c_le) against the constant residue
 * 0x2144DF1C.
 *
 * The property under test: with generator polynomial 0x04C11DB7, register
 * init 0xFFFFFFFF, final xor 0xFFFFFFFF, and reflected input/output, the
 * CRC of a message carrying its own check bytes is a fixed value that
 * does not depend on the message. Appending the check bytes in the wrong
 * byte order does not produce it (negative control).
 */
#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crc32.h"

#define RESIDUE 0x2144DF1CUL
#define CHECK_VECTOR 0xCBF43926UL   /* crc32("123456789") */
#define NCASES 1000000UL
#define MAXLEN 256
#define POOL_BYTES (16UL * 1024 * 1024)

/* splitmix64, fixed seed (the proof-engine convention). */
static uint64_t sm_state;

static void sm_seed(uint64_t seed)
{
    sm_state = seed;
}

static uint64_t sm_next(void)
{
    uint64_t z = (sm_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

/* FNV-1a 64, folded over (length, crc, residue) per case. */
static uint64_t fnv;

static void fnv_fold(const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        fnv ^= p[i];
        fnv *= 0x100000001B3ULL;
    }
}

static void fnv_u32(uint32_t v)
{
    uint8_t b[4];
    b[0] = (uint8_t)v;
    b[1] = (uint8_t)(v >> 8);
    b[2] = (uint8_t)(v >> 16);
    b[3] = (uint8_t)(v >> 24);
    fnv_fold(b, 4);
}

/* Append c to buf[len] in little-endian (wire) order. */
static void append_le(uint8_t *buf, size_t len, uint32_t c)
{
    buf[len] = (uint8_t)c;
    buf[len + 1] = (uint8_t)(c >> 8);
    buf[len + 2] = (uint8_t)(c >> 16);
    buf[len + 3] = (uint8_t)(c >> 24);
}

static int failures = 0;

static void expect_u32(const char *what, uint32_t got, uint32_t want)
{
    if (got != want) {
        printf("FAIL %s: got 0x%08" PRIx32 ", want 0x%08" PRIx32 "\n",
               what, got, want);
        failures++;
    } else {
        printf("ok   %s: 0x%08" PRIx32 "\n", what, got);
    }
}

int main(void)
{
    /* ---- phase 1: known-answer vector + residue on the vector ---- */
    const char *vec = "123456789";
    uint32_t c_tab = crc32_table(vec, 9);
    uint32_t c_bit = crc32_bitwise(vec, 9);
    expect_u32("crc32(\"123456789\") table", c_tab, (uint32_t)CHECK_VECTOR);
    expect_u32("crc32(\"123456789\") bitwise == table", c_bit, c_tab);

    uint8_t ext[9 + 4];
    memcpy(ext, vec, 9);
    append_le(ext, 9, c_tab);
    expect_u32("residue over \"123456789\"+crc (LE)", crc32_table(ext, 13),
               (uint32_t)RESIDUE);

    /* negative control: big-endian append must not give the residue */
    uint8_t ext_be[9 + 4];
    memcpy(ext_be, vec, 9);
    ext_be[9] = (uint8_t)(c_tab >> 24);
    ext_be[10] = (uint8_t)(c_tab >> 16);
    ext_be[11] = (uint8_t)(c_tab >> 8);
    ext_be[12] = (uint8_t)c_tab;
    uint32_t r_be = crc32_table(ext_be, 13);
    if (r_be == (uint32_t)RESIDUE) {
        printf("FAIL big-endian append gave the residue (should not happen)\n");
        failures++;
    } else {
        printf("ok   wrong byte order (BE) gives 0x%08" PRIx32
               ", not the residue\n", r_be);
    }

    /* ---- phase 2: empty message ---- */
    uint32_t c_empty = crc32_table("", 0);
    expect_u32("crc32(empty)", c_empty, 0x00000000UL);
    uint8_t ext0[4];
    append_le(ext0, 0, c_empty);
    expect_u32("residue over empty+4 zero bytes", crc32_table(ext0, 4),
               (uint32_t)RESIDUE);

    /* ---- phase 3: 1,000,000 fixed-seed random buffers ---- */
    fnv = 0xCBF29CE484222325ULL;
    sm_seed(0x123456789ABCDEF0ULL);
    uint8_t buf[MAXLEN + 4];
    uint64_t residue_fail = 0, oracle_fail = 0, len0 = 0;
    for (uint64_t i = 0; i < NCASES; i++) {
        size_t len = (size_t)(sm_next() % (MAXLEN + 1));
        for (size_t j = 0; j < len; j++)
            buf[j] = (uint8_t)(sm_next() >> 32);
        if (len == 0)
            len0++;
        uint32_t c = crc32_table(buf, len);
        if (crc32_bitwise(buf, len) != c)
            oracle_fail++;
        append_le(buf, len, c);
        uint32_t r = crc32_table(buf, len + 4);
        if (r != (uint32_t)RESIDUE)
            residue_fail++;
        fnv ^= (uint8_t)len;
        fnv *= 0x100000001B3ULL;
        fnv_u32(c);
        fnv_u32(r);
    }
    printf("cases: %" PRIu64 " (length-0 cases: %" PRIu64 ")\n", NCASES, len0);
    printf("residue mismatches: %" PRIu64 "\n", residue_fail);
    printf("oracle disagreements (table vs bitwise): %" PRIu64 "\n",
           oracle_fail);
    printf("fnv64 over per-case results: 0x%016" PRIx64 "\n", fnv);
    if (residue_fail || oracle_fail)
        failures++;

    /* ---- phase 4: throughput of the two-pass flow at this build ---- */
    uint8_t *pool = malloc(POOL_BYTES);
    if (!pool) {
        printf("FAIL malloc pool\n");
        return 1;
    }
    sm_seed(0x123456789ABCDEF0ULL);
    for (size_t i = 0; i < POOL_BYTES; i += 8) {
        uint64_t w = sm_next();
        memcpy(pool + i, &w, 8);
    }
    double best = 0.0;
    uint64_t sum_first = 0, sum_second = 0;
    for (int rep = 0; rep < 3; rep++) {
        sm_seed(0x123456789ABCDEF0ULL);
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        uint64_t s1 = 0, s2 = 0;
        uint8_t scratch[MAXLEN + 4];
        for (uint64_t i = 0; i < NCASES; i++) {
            size_t len = (size_t)(sm_next() % (MAXLEN + 1));
            size_t off = (size_t)(sm_next() % (POOL_BYTES - MAXLEN - 4));
            memcpy(scratch, pool + off, len);
            uint32_t c = crc32_table(scratch, len);
            append_le(scratch, len, c);
            volatile uint32_t r = crc32_table(scratch, len + 4);
            (void)r;
            s1 += len;
            s2 += len + 4;
        }
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double dt = (t1.tv_sec - t0.tv_sec) +
                    (t1.tv_nsec - t0.tv_nsec) * 1e-9;
        double mib_s = ((s1 + s2) / (1024.0 * 1024.0)) / dt;
        if (mib_s > best) {
            best = mib_s;
            sum_first = s1;
            sum_second = s2;
        }
    }
    free(pool);
    printf("throughput (two-pass: checksum, append, re-checksum over "
           "%" PRIu64 " buffers, best of 3):\n", NCASES);
    printf("  bytes checksummed: pass1=%" PRIu64 " pass2=%" PRIu64 "\n",
           sum_first, sum_second);
    printf("  %.1f MiB/s (%.2f GiB/s)\n", best, best / 1024.0);

    if (failures) {
        printf("RESULT: FAIL (%d)\n", failures);
        return 1;
    }
    printf("RESULT: ALL TESTS PASSED\n");
    return 0;
}
