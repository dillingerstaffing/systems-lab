/* Unsigned 32-bit division by 7, computed from the binary expansion of the
 * reciprocal of 7.
 *
 * In binary, the reciprocal of 7 is the repeating fraction 0.001001001...,
 * because the geometric series 2^-3 + 2^-6 + 2^-9 + ... with ratio 2^-3
 * sums to exactly the reciprocal of 7. Multiplying through by x:
 *
 *   x divided by 7 = (x >> 3) + (x >> 6) + (x >> 9) + ...   (as exact reals)
 *
 * Keeping the integer part of each term gives the estimate
 *
 *   q = (x >> 3) + (x >> 6) + ... + (x >> 30),
 *
 * which never exceeds x divided by 7 and falls short of it by less than
 * 10.58 (proof in PROOF.md), so q starts at most 10 below the true
 * quotient. A short correction loop then walks q up to the exact value,
 * comparing 7q, built as q + (q << 1) + (q << 2), against x. No division
 * or multiplication operators appear anywhere in this file.
 */
#ifndef DIV7_SHIFTADD_H
#define DIV7_SHIFTADD_H

#include <stdint.h>

static uint32_t udiv7_shiftadd(uint32_t x)
{
    uint32_t q = (x >> 3) + (x >> 6) + (x >> 9) + (x >> 12) + (x >> 15)
               + (x >> 18) + (x >> 21) + (x >> 24) + (x >> 27) + (x >> 30);
    uint32_t t = q + (q << 1) + (q << 2); /* t = 7q, shifts and adds only */

    /* q never exceeds x divided by 7, hence t never exceeds x, so (x - t)
     * cannot underflow. Each pass moves q one step toward the quotient. */
    while ((x - t) >= 7u) {
        q += 1u;
        t += 7u;
    }
    return q;
}

#endif /* DIV7_SHIFTADD_H */
