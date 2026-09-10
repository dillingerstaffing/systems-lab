#ifndef DIV3_MAGIC_H
#define DIV3_MAGIC_H

#include <stdint.h>

/*
 * div3_u32(x): floor(x / 3) for x in [0, 2^32), computed as
 *
 *     q = (uint32_t)(((uint64_t)x * 0xAAAAAAABULL) >> 33)
 *
 * with no division operator in the implementation.
 *
 * Why this is exact, from integer arithmetic alone. Let
 * M = 0xAAAAAAAB. Note 2^33 mod 3 = 2 (since 4^k = 1 mod 3, so
 * 2^33 = 2 * 4^16 = 2 mod 3), hence M = (2^33 + 1) / 3 = 2863311531.
 *
 * Write x = 3a + r with r in {0, 1, 2}. Then
 *
 *     x * M = (3a + r)(2^33 + 1) / 3
 *           = a * 2^33 + a + r * (2^33 + 1) / 3.
 *
 * Dividing by 2^33 and flooring:
 *
 *     floor(x * M / 2^33) = a + floor(a / 2^33 + r * (2^33 + 1) / (3 * 2^33)).
 *
 * The fractional term: a <= (2^32 - 1) / 3 < 2^31, so
 * a / 2^33 < 1 / 6, and r * (2^33 + 1) / (3 * 2^33) = r / 3 + r / (3 * 2^33)
 * < r / 3 + 1 / 2^32. For r = 2 (the largest case) the sum is
 * < 1/6 + 2/3 + 1/2^32 < 1, and it is smaller for r = 0, 1. So the
 * floor of the fractional part is 0 and floor(x * M / 2^33) = a =
 * floor(x / 3) exactly, for every x in [0, 2^32).
 *
 * The product x * M fits in 64 bits: x < 2^32 and M < 2^32, so
 * x * M < 2^64 and (uint64_t)x * 0xAAAAAAABULL is exact, with no
 * wraparound before the shift.
 */
uint32_t div3_u32(uint32_t x);

#endif
