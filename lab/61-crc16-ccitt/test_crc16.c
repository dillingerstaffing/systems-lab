#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "crc16.h"

#define NBUFFERS 1000000
#define MAXLEN 256
#define THROUGHPUT_MIB 64

/* Naive bit reversal, independent of any builtin or table. */
static uint8_t rev8(uint8_t b)
{
    uint8_t r = 0;
    for (int i = 0; i < 8; i++) {
        r = (uint8_t)((r << 1) | (b & 1u));
        b >>= 1;
    }
    return r;
}

static uint16_t rev16(uint16_t x)
{
    uint16_t r = 0;
    for (int i = 0; i < 16; i++) {
        r = (uint16_t)((r << 1) | (x & 1u));
        x >>= 1;
    }
    return r;
}

/*
 * Independent reference, MSB-first long division with the unreflected
 * polynomial 0x1021. Different bit order, different loop, different
 * polynomial constant from crc16_ccitt(); the two share only the
 * parameter set (poly 0x1021, init 0x0000, xorout 0x0000). On the raw
 * message this is the CRC-16/XMODEM algorithm; run over the
 * bit-reversed message with the result bit-reversed, it must equal
 * the reflected implementation for every input.
 */
static uint16_t ref_msb_raw(const uint8_t *data, size_t len)
{
    uint16_t reg = 0;
    for (size_t i = 0; i < len; i++) {
        reg ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int k = 0; k < 8; k++)
            reg = (uint16_t)((reg & 0x8000u) ? (uint16_t)((reg << 1) ^ 0x1021u)
                                             : (uint16_t)(reg << 1));
    }
    return reg;
}

static uint16_t ref_crc16(const uint8_t *data, size_t len)
{
    uint16_t reg = 0;
    for (size_t i = 0; i < len; i++) {
        reg ^= (uint16_t)((uint16_t)rev8(data[i]) << 8);
        for (int k = 0; k < 8; k++)
            reg = (uint16_t)((reg & 0x8000u) ? (uint16_t)((reg << 1) ^ 0x1021u)
                                             : (uint16_t)(reg << 1));
    }
    return rev16(reg);
}

/* Fixed-seed splitmix64 so every run draws the same stream. */
static uint64_t splitmix64(uint64_t *state)
{
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static double now_sec(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

static int check_vector(const char *name, const uint8_t *data, size_t len,
                        uint16_t expect)
{
    uint16_t got = crc16_ccitt(data, len);
    uint16_t ref = ref_crc16(data, len);
    int ok = (got == expect) && (ref == expect);
    printf("vector %-12s expect=0x%04x got=0x%04x ref=0x%04x %s\n",
           name, expect, got, ref, ok ? "OK" : "FAIL");
    return ok ? 0 : 1;
}

int main(void)
{
    int failures = 0;

    /* Published vectors. "123456789" -> 0x2189 is the CRC catalogue's
       CRC-16/KERMIT check value (poly 0x1021, refin/refout true,
       init/xorout 0x0000). The 6-byte message reproduces a published
       XMODEM value (0x7DCC) through the MSB-first reference; its KERMIT
       value 0xA672 is pinned by the implementation, the MSB-first
       reference, and an independent Python recurrence (see PROOF.md). */
    failures += check_vector("empty", NULL, 0, 0x0000);
    failures += check_vector("\"123456789\"",
                             (const uint8_t *)"123456789", 9, 0x2189);
    failures += check_vector("{0x01}", (const uint8_t *)"\x01", 1, 0x1189);
    {
        static const uint8_t m[6] = { 0x05, 0x01, 0x00, 0x04, 0xFC, 0xFF };
        uint16_t raw = ref_msb_raw(m, 6);
        int ok = (raw == 0x7DCC);
        printf("vector xmodem-ref  expect=0x7dcc got=0x%04x %s\n",
               raw, ok ? "OK" : "FAIL");
        failures += ok ? 0 : 1;
        /* A forum post quoting this message pair claims KERMIT = 0x72A6,
           but 0x72A6 is the byte-swapped form of the value all three
           independent recurrences here (and a Python cross-check)
           produce, 0xA672. The poster's value carries an extra byte swap
           and is not used; the canonical value is asserted instead. */
        failures += check_vector("kermit-6byte", m, 6, 0xA672);
    }

    /* Differential test: 1M fixed-seed buffers, lengths 0..256 drawn
       uniformly (covers 0, 1, and odd lengths). */
    uint64_t state = 0xC16CC177A16CC177ULL;
    uint64_t fnv = 0xCBF29CE484222325ULL; /* FNV-1a 64-bit offset basis */
    uint64_t mismatches = 0, bytes_total = 0;
    uint8_t buf[MAXLEN];

    double t0 = now_sec();
    for (uint64_t n = 0; n < NBUFFERS; n++) {
        size_t len = (size_t)(splitmix64(&state) % (MAXLEN + 1));
        for (size_t i = 0; i < len; i += 8) {
            uint64_t w = splitmix64(&state);
            size_t chunk = (len - i < 8) ? (len - i) : 8;
            memcpy(buf + i, &w, chunk);
        }
        uint16_t got = crc16_ccitt(buf, len);
        uint16_t ref = ref_crc16(buf, len);
        if (got != ref) {
            mismatches++;
            if (mismatches <= 5)
                printf("mismatch: len=%zu got=0x%04x ref=0x%04x\n",
                       len, got, ref);
        }
        fnv ^= got;
        fnv *= 0x100000001B3ULL;
        bytes_total += len;
    }
    double t1 = now_sec();

    printf("differential: %u buffers, %llu bytes total, mismatches=%llu\n",
           NBUFFERS, (unsigned long long)bytes_total,
           (unsigned long long)mismatches);
    printf("fnv1a=%016llx\n", (unsigned long long)fnv);
    printf("verification_time=%.3f s\n", t1 - t0);

    /* Throughput: 64 MiB of crc16_ccitt passes over a fixed 1 MiB
       buffer, results xor-folded into a printed accumulator so the
       loop is not dead code. */
    static uint8_t big[1 << 20];
    for (size_t i = 0; i < sizeof big; i++)
        big[i] = (uint8_t)(splitmix64(&state) & 0xFF);
    uint16_t acc = 0;
    double u0 = now_sec();
    for (int r = 0; r < THROUGHPUT_MIB; r++)
        acc ^= crc16_ccitt(big, sizeof big);
    double u1 = now_sec();
    double mib = (double)THROUGHPUT_MIB;
    printf("throughput: %.1f MiB in %.3f s = %.1f MiB/s (acc=0x%04x)\n",
           mib, u1 - u0, mib / (u1 - u0), acc);

    printf("RESULT: %s\n",
           (failures == 0 && mismatches == 0) ? "PASS" : "FAIL");
    return (failures == 0 && mismatches == 0) ? 0 : 1;
}
