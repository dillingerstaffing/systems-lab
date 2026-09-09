/* minmax.h — branchless min/max over the full int32_t domain.
 *
 * The selection bit is the sign of the exact difference d = a - b.
 * The difference is computed in 64-bit arithmetic and observed through
 * an unsigned 64-bit value, so every operation below is defined for all
 * int32_t inputs: the 64-bit intermediate holds the exact difference
 * for every pair (no 64-bit value can overflow when both operands fit
 * in 32 bits), the ">> 63" shift and the unary minus act on unsigned
 * values (well-defined), and the final select is pure bitwise logic.
 *
 * Stated plainly: there is no signed 32-bit subtraction anywhere in
 * these functions, so no signed-overflow assumption is needed at all.
 * A 32-bit "mask = (a - b) >> 31" formulation would be wrong near the
 * INT32_MIN/INT32_MAX boundary (the wrapped difference of
 * INT32_MIN - INT32_MAX is +1, positive), which is why the difference
 * is taken wide here.
 */
#ifndef MINMAX_H
#define MINMAX_H

#include <stdint.h>

static inline int32_t bmin32(int32_t a, int32_t b)
{
    /* d wraps modulo 2^64; bit 63 is set exactly when a < b. */
    uint64_t d = (uint64_t)(int64_t)a - (uint64_t)(int64_t)b;
    uint64_t m = -(d >> 63); /* all ones iff a < b, else zero */
    uint64_t ua = (uint64_t)(uint32_t)a;
    uint64_t ub = (uint64_t)(uint32_t)b;
    return (int32_t)(uint32_t)((ua & m) | (ub & ~m));
}

static inline int32_t bmax32(int32_t a, int32_t b)
{
    uint64_t d = (uint64_t)(int64_t)a - (uint64_t)(int64_t)b;
    uint64_t m = -(d >> 63); /* all ones iff a < b, else zero */
    uint64_t ua = (uint64_t)(uint32_t)a;
    uint64_t ub = (uint64_t)(uint32_t)b;
    return (int32_t)(uint32_t)((ub & m) | (ua & ~m));
}

#endif
