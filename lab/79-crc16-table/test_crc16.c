/*
 * test_crc16.c: verification harness for lab/79-crc16-table.
 *
 * 1. Re-derives all 256 table entries from the polynomial with a serial
 *    divider written independently of crc16.h (24-bit dividend: the 8 bits
 *    of the byte followed by 16 zero bits, divided by x^16 + x^12 + x^5 + 1
 *    one bit at a time) and requires them to match the startup-derived
 *    table exactly.
 * 2. Checks known-answer vectors through both implementations.
 * 3. Differentially tests the bitwise and table-driven implementations on
 *    all 256 single-byte messages and 1,000,000 fixed-seed random buffers
 *    (lengths 0..256): 0 mismatches required. An FNV-1a checksum over every
 *    result must be identical across all builds.
 * 4. Measures throughput of both implementations at -O2 (timed loop is
 *    pure CRC passes over a buffer filled once before timing; no PRNG step
 *    inside the timed region).
 *
 * Usage: ./test_crc16            run the full harness
 *        ./test_crc16 --dump-table  print the 256 derived entries, one per line
 */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crc16.h"

/* Serial polynomial divider, independent of crc16.h's derivation loop:
 * feeds the 24-bit dividend (byte bits MSB first, then 16 zeros) through
 * the x^16 + x^12 + x^5 + 1 divider one bit per iteration. The final
 * register is the remainder of (byte(x) * x^16) / poly, which is exactly
 * what table[i] must hold. */
static uint16_t derive_entry_ref(unsigned byte)
{
    uint16_t reg = 0;
    for (int k = 23; k >= 0; k--) {
        unsigned bit = (k >= 16) ? ((byte >> (k - 16)) & 1u) : 0u;
        unsigned top = (reg >> 15) & 1u;
        reg = (uint16_t)(((reg << 1) | bit) & 0xFFFFu);
        if (top)
            reg ^= 0x1021u;
    }
    return reg;
}

/* splitmix64, fixed seed: the only randomness in the harness, and it is
 * identical on every run. */
static uint64_t rng_state = 0x9E3779B97F4A7C15u;

static uint64_t rng_next(void)
{
    uint64_t z = (rng_state += 0x9E3779B97F4A7C15u);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9u;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBu;
    return z ^ (z >> 31);
}

/* FNV-1a 64-bit over every measured output. */
static uint64_t fnv = 0xCBF29CE484222325u;

static void fnv_feed_u16(uint16_t v)
{
    fnv ^= (uint64_t)(v & 0xFFu);
    fnv *= 0x100000001B3u;
    fnv ^= (uint64_t)((v >> 8) & 0xFFu);
    fnv *= 0x100000001B3u;
}

static int failures = 0;

static void check_vector(const char *name, uint16_t expect,
                         const uint8_t *data, size_t len)
{
    uint16_t got_bit = crc16_bitwise(data, len);
    uint16_t got_tab = crc16_table(data, len);
    int ok = (got_bit == expect) && (got_tab == expect) && (got_bit == got_tab);
    if (!ok)
        failures++;
    printf("vector %-14s expect=0x%04x bitwise=0x%04x table=0x%04x %s\n",
           name, expect, got_bit, got_tab, ok ? "OK" : "FAIL");
}

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
    crc16_table_init();

    if (argc == 2 && strcmp(argv[1], "--dump-table") == 0) {
        for (int i = 0; i < 256; i++)
            printf("%04x\n", crc16_table_state[i]);
        return 0;
    }

    /* 1. Table derivation: independent re-derivation vs startup table. */
    int tab_ok = 0;
    for (int i = 0; i < 256; i++) {
        uint16_t ref = derive_entry_ref((unsigned)i);
        if (ref != crc16_table_state[i]) {
            printf("table[%d]: derived=0x%04x rederived=0x%04x FAIL\n",
                   i, crc16_table_state[i], ref);
            failures++;
        } else {
            tab_ok++;
        }
        fnv_feed_u16(crc16_table_state[i]);
    }
    printf("table derivation: %d/256 entries match independent re-derivation\n",
           tab_ok);

    /* 2. Known-answer vectors through both implementations. */
    static const uint8_t msg9[] = "123456789";
    static uint8_t empty[1];
    check_vector("\"123456789\"", 0x29B1u, msg9, 9);
    check_vector("empty", 0xFFFFu, empty, 0);

    /* 3. Differential: all 256 single bytes, then 1M fixed-seed buffers. */
    uint64_t messages = 0;
    uint64_t bytes_total = 0;
    uint64_t mismatches = 0;
    uint8_t buf[256];

    for (int i = 0; i < 256; i++) {
        buf[0] = (uint8_t)i;
        uint16_t a = crc16_bitwise(buf, 1);
        uint16_t b = crc16_table(buf, 1);
        if (a != b)
            mismatches++;
        fnv_feed_u16(a);
        fnv_feed_u16(b);
        messages++;
        bytes_total++;
    }

    double t0 = now_s();
    for (uint64_t n = 0; n < 1000000u; n++) {
        size_t len = (size_t)(rng_next() % 257u);
        for (size_t i = 0; i < len; i++)
            buf[i] = (uint8_t)(rng_next() >> 32);
        uint16_t a = crc16_bitwise(buf, len);
        uint16_t b = crc16_table(buf, len);
        if (a != b)
            mismatches++;
        fnv_feed_u16(a);
        fnv_feed_u16(b);
        messages++;
        bytes_total += len;
    }
    double verification_time = now_s() - t0;
    printf("differential: %llu messages, %llu bytes total, mismatches=%llu\n",
           (unsigned long long)messages, (unsigned long long)bytes_total,
           (unsigned long long)mismatches);
    if (mismatches != 0)
        failures++;
    printf("fnv1a=%016llx\n", (unsigned long long)fnv);
    printf("verification_time=%.3f s\n", verification_time);

    /* 4. Throughput: 64 passes over a 1 MiB buffer filled once before the
     * timed region, so the timed loop contains no PRNG step. Results are
     * xor-folded into a printed accumulator so the calls are not dead code. */
    static uint8_t bench[1 << 20];
    for (size_t i = 0; i < sizeof bench; i++)
        bench[i] = (uint8_t)(rng_next() >> 32);

    const int passes = 64;
    const double mib = (double)(passes * sizeof bench) / (1 << 20);

    uint16_t acc = 0;
    t0 = now_s();
    for (int p = 0; p < passes; p++)
        acc ^= crc16_bitwise(bench, sizeof bench);
    double dt = now_s() - t0;
    printf("throughput bitwise: %.1f MiB in %.3f s = %.1f MiB/s (acc=0x%04x)\n",
           mib, dt, mib / dt, acc);

    acc = 0;
    t0 = now_s();
    for (int p = 0; p < passes; p++)
        acc ^= crc16_table(bench, sizeof bench);
    dt = now_s() - t0;
    printf("throughput table:   %.1f MiB in %.3f s = %.1f MiB/s (acc=0x%04x)\n",
           mib, dt, mib / dt, acc);

    printf("RESULT: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}
