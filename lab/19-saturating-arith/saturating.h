#ifndef SATURATING_H
#define SATURATING_H

#include <stdint.h>

/*
 * sat_add32 / sat_sub32: 32-bit integer addition and subtraction with
 * saturation. The result is computed in unsigned arithmetic (where
 * wraparound is defined) and the overflow condition is read off the
 * three sign bits. On overflow the result clamps to INT32_MAX or
 * INT32_MIN instead of wrapping.
 */
int32_t sat_add32(int32_t a, int32_t b);
int32_t sat_sub32(int32_t a, int32_t b);

#endif
