#ifndef FIXED_POINT_H
#define FIXED_POINT_H

#include <stdint.h>

/*
 * Q16.16 fixed point: an int32_t whose value is raw / 65536.
 *
 * Representable range: [-32768, 32767 + 65535/65536]
 *   = [INT32_MIN / 65536.0, INT32_MAX / 65536.0].
 * One ulp (least significant bit) = 1/65536.
 *
 * q16_add(a, b): returns (int32_t)((uint32_t)a + (uint32_t)b).
 * Unsigned addition is defined to wrap modulo 2^32, so a sum outside
 * the representable range wraps around instead of saturating. This
 * matches plain C unsigned arithmetic; callers that need saturation
 * must range-check before calling.
 *
 * q16_mul(a, b): the exact product (int64_t)a * (int64_t)b cannot
 * overflow (|product| <= 2^62 < 2^63). It is rounded to the nearest
 * Q16.16 value, ties away from zero:
 *   product >= 0 : ((product + 32768) >> 16)
 *   product <  0 : -(((-product + 32768) >> 16))
 * The shifts run on non-negative int64_t values (always defined),
 * and the final narrowing to int32_t goes through uint32_t, which is
 * defined to wrap modulo 2^32. A rounded product outside the
 * representable range therefore wraps instead of saturating.
 */
int32_t q16_add(int32_t a, int32_t b);
int32_t q16_mul(int32_t a, int32_t b);

#endif
