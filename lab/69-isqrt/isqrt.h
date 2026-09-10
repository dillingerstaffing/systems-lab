#ifndef ISQRT_H
#define ISQRT_H

#include <stdint.h>

/*
 * isqrt64: integer square root of a 64-bit unsigned value.
 *
 * Returns the largest r with r*r <= n.
 *
 * Construction: digit-by-digit (restoring) square root in base 4. Each of
 * the 32 loop iterations decides one more bit of the answer, from the most
 * significant bit down. The trial value for the next bit is tested with
 * the identity (res + bit)^2 - res^2 = 2*res*bit + bit^2, rearranged into
 * the restoring form: if the remaining value n covers res + bit, subtract
 * it and set the bit in the answer; otherwise leave the answer's bit clear.
 * The answer is built shifted: res is halved each round while bit steps
 * down by powers of 4, which is exactly the digit-by-digit recurrence.
 *
 * Bounds (all unsigned, so no UB regardless): bit starts at 2^62 and only
 * shrinks; res satisfies res < 2^33 at every iteration (each round at most
 * halves res then adds bit <= 2^62, and the closed form of that recurrence
 * from res = 0 stays below 2^33), so res + bit < 2^64 and every
 * add/subtract/shift stays inside 64 bits.
 *
 * Only unsigned 64-bit add, subtract, compare, and shift. No floating
 * point, no libm, no compiler sqrt builtins.
 */
static inline uint64_t isqrt64(uint64_t n)
{
    uint64_t res = 0;
    uint64_t bit = UINT64_C(1) << 62; /* highest power of 4 below 2^64 */

    while (bit != 0) {
        if (n >= res + bit) {
            n -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res >>= 1;
        }
        bit >>= 2;
    }
    return res;
}

#endif /* ISQRT_H */
