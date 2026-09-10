#ifndef SAT_ADD32_H
#define SAT_ADD32_H

#include <stdint.h>

/*
 * sat_add32: int32_t addition that clamps to INT32_MAX / INT32_MIN
 * on signed overflow instead of wrapping.
 *
 * Construction. The sum is formed in unsigned arithmetic, where
 * wraparound is defined and never undefined behavior. Overflow is
 * detected from the sign bits alone, using the two's complement
 * identity: (a + b) overflows exactly when the two addends share a
 * sign and the wrapped sum carries the opposite sign. With mixed
 * signs the true sum lies strictly between the two operands, so
 * overflow is impossible.
 *
 * ov has its sign bit set exactly on overflow:
 *   ((ua ^ sum) & (ub ^ sum)) >> 31 == 1  iff  overflow.
 * Proof of the identity: ua ^ sum differs in bit 31 exactly when
 * sign(a) != sign(sum), likewise for b. Both hold exactly when
 * sign(a) == sign(b) != sign(sum), the overflow condition.
 *
 * On overflow both addends share one sign, so the clamp value is
 * chosen from sign(a): INT32_MIN for negative addends, INT32_MAX
 * for positive ones. The select is branchless: mask is all-zeros
 * when there is no overflow and all-ones when there is.
 */
static inline int32_t sat_add32(int32_t a, int32_t b) {
    uint32_t ua = (uint32_t)a;
    uint32_t ub = (uint32_t)b;
    uint32_t sum = ua + ub;                     /* wraps, defined */
    uint32_t ov = (ua ^ sum) & (ub ^ sum);      /* sign bit set iff overflow */
    uint32_t m = (uint32_t)(-(int32_t)(ov >> 31)); /* ~0u on overflow, 0 else */

    /* INT32_MIN for negative addends, INT32_MAX for positive ones.
       INT32_MIN == INT32_MAX ^ 0xFFFFFFFF, so xor with the all-ones
       mask derived from sign(a) selects between them. */
    uint32_t clamped = (uint32_t)INT32_MAX ^ (uint32_t)(-(int32_t)(ua >> 31));

    return (int32_t)((sum & ~m) | (clamped & m));
}

#endif
