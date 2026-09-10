/* lab/53-double-dabble test driver.
 *
 * Verification:
 *  1. Exhaustive identity check: for every uint8_t v, unpack the result
 *     into hundreds/tens/ones and require each nibble in 0..9 and
 *     hundreds*100 + tens*10 + ones == v.
 *  2. Differential test: snprintf "%03u" is the oracle; every digit of
 *     every result must match it. snprintf is used only as the oracle
 *     here, never in the implementation.
 *  3. Throughput: 100,000,000 values at -O2, timed with
 *     CLOCK_MONOTONIC. Inputs come from an xorshift32 step (noted below)
 *     so the compiler cannot fold the conversions; the accumulator is
 *     printed so the work cannot be discarded.
 */
#define _POSIX_C_SOURCE 200809L /* for clock_gettime under -std=c11 */

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "dabble.h"

static uint32_t xorshift32(uint32_t *s)
{
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

int main(void)
{
    long total_checks = 0;
    long mismatches = 0;

    for (unsigned v = 0; v < 256; v++) {
        uint16_t b = dabble_to_bcd((uint8_t)v);
        unsigned h = (unsigned)((b >> 8) & 0xF);
        unsigned t = (unsigned)((b >> 4) & 0xF);
        unsigned o = (unsigned)(b & 0xF);

        /* Nibble layout: each of the three nibbles must be a decimal digit. */
        if (h > 9 || t > 9 || o > 9) {
            printf("nibble out of range: v=%u b=0x%03x\n", v, b);
            mismatches++;
        }
        total_checks += 3;

        /* Identity: unpacked digits must reconstruct the input exactly. */
        if (h * 100 + t * 10 + o != v) {
            printf("identity mismatch: v=%u b=0x%03x\n", v, b);
            mismatches++;
        }
        total_checks++;

        /* Oracle: snprintf "%03u" digits must all match. */
        char ref[4];
        snprintf(ref, sizeof ref, "%03u", v);
        if ((unsigned)(ref[0] - '0') != h ||
            (unsigned)(ref[1] - '0') != t ||
            (unsigned)(ref[2] - '0') != o) {
            printf("oracle mismatch: v=%u b=0x%03x ref=%s\n", v, b, ref);
            mismatches++;
        }
        total_checks++;
    }

    /* Throughput at -O2: 100M conversions. The xorshift32 step below is
     * part of the timed loop; inputs are runtime data, not constants. */
    const long N = 100000000L;
    uint32_t state = 0x12345678u;
    uint32_t acc = 0;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (long i = 0; i < N; i++) {
        uint8_t v = (uint8_t)(xorshift32(&state) & 0xFFu);
        acc ^= dabble_to_bcd(v);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double secs = (double)(t1.tv_sec - t0.tv_sec) +
                  (double)(t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("exhaustive_inputs=256\n");
    printf("total_checks=%ld\n", total_checks);
    printf("mismatches=%ld\n", mismatches);
    printf("throughput_values=%ld\n", N);
    printf("throughput_time_s=%.3f\n", secs);
    printf("throughput_ns_per_value=%.3f\n", secs * 1e9 / (double)N);
    printf("throughput_acc=%08x\n", acc);
    return mismatches == 0 ? 0 : 1;
}
