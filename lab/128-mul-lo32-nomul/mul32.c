#include "mul32.h"

/*
 * mul16: unsigned 16-bit by 16-bit multiply, result exact in 32 bits.
 *
 * Shift-add: for each set bit i of y, add x shifted left by i.
 * No multiply operator anywhere in this file.
 */
static uint32_t mul16(uint16_t x, uint16_t y)
{
    uint32_t r = 0;
    uint32_t xx = (uint32_t)x;
    for (unsigned i = 0; i < 16; i++) {
        if (((y >> i) & 1u) != 0)
            r += xx;
        xx <<= 1;
    }
    return r;
}

/*
 * mullo32: low 32 bits of the 64-bit product a * b.
 *
 * Only the low 32 bits of each operand can affect the low 32 bits of
 * the product: write A = a mod 2^32 = a0 + a1 * 2^16 and
 * B = b mod 2^32 = b0 + b1 * 2^16 with 16-bit halves a0, a1, b0, b1.
 * The schoolbook expansion is
 *   A * B = a0*b0 + (a0*b1 + a1*b0) * 2^16 + a1*b1 * 2^32.
 * Modulo 2^32 the a1*b1 term vanishes, and of the middle term only its
 * low 16 bits survive the 2^16 scaling. Hence
 *   low32 = p0 + (((p1 mod 2^16) + (p2 mod 2^16)) mod 2^16) * 2^16
 * where p0 = a0*b0, p1 = a0*b1, p2 = a1*b0, each exact in 32 bits.
 * Adding the masked-shifted terms one at a time in mod-2^32 unsigned
 * arithmetic performs the inner mod 2^16 for free.
 */
uint32_t mullo32(uint64_t a, uint64_t b)
{
    uint32_t al = (uint32_t)a;
    uint32_t bl = (uint32_t)b;
    uint16_t a0 = (uint16_t)al;
    uint16_t a1 = (uint16_t)(al >> 16);
    uint16_t b0 = (uint16_t)bl;
    uint16_t b1 = (uint16_t)(bl >> 16);

    uint32_t p0 = mul16(a0, b0);
    uint32_t p1 = mul16(a0, b1);
    uint32_t p2 = mul16(a1, b0);

    uint32_t r = p0;
    r += (p1 & 0xFFFFu) << 16;
    r += (p2 & 0xFFFFu) << 16;
    return r;
}
