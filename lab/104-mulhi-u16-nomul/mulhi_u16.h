#ifndef MULHI_U16_H
#define MULHI_U16_H

#include <stdint.h>

/*
 * mulhi_u16: the high 16 bits of the 32-bit product of two uint16_t
 * values, built from the schoolbook shift-add partial-product identity
 * and nothing else.
 *
 * Write b in binary as b = sum_{i=0..15} bit_i(b) * 2^i. Then
 *
 *     a * b = sum_{i=0..15} bit_i(b) * (a << i),
 *
 * so the full 32-bit product is the sum of the shifted copies of a
 * at the positions where b has a 1 bit. The loop walks the bits of
 * b from low to high, keeping x = a << i and adding x into a 32-bit
 * accumulator exactly when bit i of b is set. No multiply operator
 * appears anywhere below: the only operations are tests, shifts,
 * and adds, all on unsigned values, so every step is well-defined
 * (the accumulator never exceeds (2^16 - 1)^2 < 2^32, and x never
 * exceeds a << 16 < 2^32).
 *
 * The return value is the top half of that 32-bit product. The test
 * (test_mulhi_u16.c) checks it against ((uint32_t)a * b) >> 16 over
 * all 2^32 input pairs; the multiplication there is the oracle only
 * and never appears in this file.
 */
static uint16_t mulhi_u16(uint16_t a, uint16_t b)
{
    uint32_t acc = 0;
    uint32_t x = a;   /* a shifted left one bit per step: a << i */
    uint16_t y = b;   /* multiplier bits, shifted right one per step */
    for (int i = 0; i < 16; i++) {
        if (y & 1u)
            acc += x;
        x <<= 1;
        y >>= 1;
    }
    return (uint16_t)(acc >> 16);
}

#endif /* MULHI_U16_H */
