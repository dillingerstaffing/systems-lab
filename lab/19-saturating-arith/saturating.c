#include "saturating.h"

#include <limits.h>
#include <stdint.h>

/*
 * The sum is formed in unsigned arithmetic, where wraparound is defined,
 * and the overflow condition is read off the three sign bits. In two's
 * complement, a + b overflows exactly when both addends share a sign and
 * the wrapped sum carries the opposite sign: two positives can only wrap
 * to a negative, two negatives can only wrap to a non-negative. With mixed
 * signs the true sum lies between the two operands, which are both inside
 * [INT32_MIN, INT32_MAX], so overflow is impossible. When no overflow
 * occurred, the wrapped sum equals the true sum and lies in
 * [INT32_MIN, INT32_MAX], so the conversion back to int32_t is in range.
 */
int32_t sat_add32(int32_t a, int32_t b)
{
    uint32_t ua = (uint32_t)a;
    uint32_t ub = (uint32_t)b;
    uint32_t ur = ua + ub;

    uint32_t sa = ua >> 31u;
    uint32_t sb = ub >> 31u;
    uint32_t sr = ur >> 31u;

    if (sa == 0u && sb == 0u && sr == 1u)
        return INT32_MAX;
    if (sa == 1u && sb == 1u && sr == 0u)
        return INT32_MIN;
    return (int32_t)ur;
}

/*
 * a - b overflows exactly when the subtrahend's sign differs from the
 * minuend's and the wrapped difference carries a sign different from the
 * minuend's: positive-minus-negative that wraps negative went past
 * INT32_MAX, negative-minus-positive that wraps non-negative went past
 * INT32_MIN. With matching signs the true difference lies between the two
 * operands, both inside [INT32_MIN, INT32_MAX], so overflow is impossible.
 * As with addition, the conversion back to int32_t only runs when the
 * wrapped difference equals the true difference, which is in range.
 */
int32_t sat_sub32(int32_t a, int32_t b)
{
    uint32_t ua = (uint32_t)a;
    uint32_t ub = (uint32_t)b;
    uint32_t ur = ua - ub;

    uint32_t sa = ua >> 31u;
    uint32_t sb = ub >> 31u;
    uint32_t sr = ur >> 31u;

    if (sa == 0u && sb == 1u && sr == 1u)
        return INT32_MAX;
    if (sa == 1u && sb == 0u && sr == 0u)
        return INT32_MIN;
    return (int32_t)ur;
}
