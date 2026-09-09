#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "xorshift16.h"

#define FULL_PERIOD 65535u /* 2^16 - 1 */

/*
 * Walk the generator from a seed until the state repeats. Track every
 * visited state in a 65536-entry byte array: a state marked twice
 * before the walk returns to the seed proves a premature repeat and
 * fails the test. Record the step count to first return and require
 * it to equal exactly 2^16 - 1, plus require that every nonzero
 * state was visited exactly once.
 *
 * Zero is asserted as a fixed point first: the recurrence maps 0 to
 * 0, so a valid full-period generator can never visit 0 from a
 * nonzero seed. If the walk ever hits 0, that is a failure.
 */
static uint32_t walk_period(uint16_t seed, uint64_t *ns_out)
{
    static uint8_t seen[65536];

    memset(seen, 0, sizeof(seen));

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    uint16_t x = seed;
    seen[x] = 1;
    uint32_t steps = 0;

    for (;;) {
        x = xorshift16_step(x);
        steps++;

        if (x == 0) {
            fprintf(stderr, "FAIL: seed 0x%04X hit state 0 after %u steps\n",
                    seed, steps);
            return 0;
        }
        if (seen[x]) {
            if (x != seed) {
                fprintf(stderr,
                        "FAIL: seed 0x%04X repeated state 0x%04X at step %u, "
                        "before returning to seed\n",
                        seed, x, steps);
                return 0;
            }
            break; /* returned to seed: the cycle is closed */
        }
        seen[x] = 1;

        if (steps > FULL_PERIOD + 1) {
            fprintf(stderr, "FAIL: seed 0x%04X never returned within %u steps\n",
                    seed, steps);
            return 0;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    *ns_out = (uint64_t)(t1.tv_sec - t0.tv_sec) * 1000000000u
            + (uint64_t)(t1.tv_nsec - t0.tv_nsec);

    /* Every nonzero state visited exactly once? Count the marks. */
    uint32_t marked = 0;
    for (uint32_t i = 0; i < 65536; i++)
        marked += seen[i];

    if (marked != FULL_PERIOD) {
        fprintf(stderr, "FAIL: seed 0x%04X visited %u distinct states, "
                "expected %u\n", seed, marked, FULL_PERIOD);
        return 0;
    }

    return steps;
}

int main(void)
{
    int failures = 0;

    /* Zero is a fixed point: the generator provably never enters or
     * leaves it, which is why the period is 2^16 - 1 and not 2^16. */
    if (xorshift16_step(0) != 0) {
        fprintf(stderr, "FAIL: step(0) != 0\n");
        failures++;
    }
    printf("fixed_point_zero=ok\n");

    /* First 8 outputs from seed 1, printed so the sequence is concrete
     * and re-checkable by hand. */
    uint16_t x = 1;
    printf("seed_0x0001_prefix=");
    for (int i = 0; i < 8; i++) {
        x = xorshift16_step(x);
        printf("%04x%c", x, i == 7 ? '\n' : ' ');
    }

    uint64_t ns = 0;
    uint32_t steps = walk_period(0x0001, &ns);
    printf("seed=0x0001 steps=%u expected=%u ns=%llu ns_per_step=%.2f\n",
           steps, FULL_PERIOD, (unsigned long long)ns,
           steps ? (double)ns / (double)steps : 0.0);
    if (steps != FULL_PERIOD) {
        fprintf(stderr, "FAIL: seed 0x0001 period %u != %u\n", steps, FULL_PERIOD);
        failures++;
    }

    /* Second, unrelated seed: the single-cycle property means every
     * nonzero seed walks the same full 65535-state cycle. */
    ns = 0;
    steps = walk_period(0xBEEF, &ns);
    printf("seed=0xbeef steps=%u expected=%u ns=%llu ns_per_step=%.2f\n",
           steps, FULL_PERIOD, (unsigned long long)ns,
           steps ? (double)ns / (double)steps : 0.0);
    if (steps != FULL_PERIOD) {
        fprintf(stderr, "FAIL: seed 0xBEEF period %u != %u\n", steps, FULL_PERIOD);
        failures++;
    }

    if (failures == 0)
        printf("PASS\n");
    else
        printf("FAILURES=%d\n", failures);

    return failures == 0 ? 0 : 1;
}
