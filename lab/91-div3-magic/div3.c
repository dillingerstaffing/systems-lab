#include "div3.h"

/*
 * Magic constant M = 0xAAAAAAAB = ceil(2^33 / 3) = (2^33 + 1) / 3.
 * Exactness proof in div3.h; in short, writing x = 3a + r, the
 * product x * M equals a * 2^33 + (a + r * M / 3) where the
 * parenthesized part is strictly below 2^33, so shifting right
 * by 33 leaves exactly a = floor(x / 3). No division operator is
 * used: only a widening multiply and a shift.
 */
uint32_t div3_u32(uint32_t x)
{
    return (uint32_t)(((uint64_t)x * 0xAAAAAAABULL) >> 33);
}
