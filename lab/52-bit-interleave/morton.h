/*
 * lab/52-bit-interleave: Morton (Z-order) codes.
 *
 * morton_interleave(lo, hi) spreads the 16 bits of each input into
 * alternating positions of a 32-bit word: bits of lo in the even
 * positions, bits of hi in the odd positions.
 *
 * morton_deinterleave(z) packs the even-position bits of z into the low
 * 16 bits and the odd-position bits into the high 16 bits of a uint32_t,
 * so deinterleave(interleave(lo, hi)) == ((uint32_t)hi << 16) | lo.
 *
 * Header-only; no tables, no builtins. All arithmetic is on uint32_t,
 * so every shift is a logical shift with no overflow risk.
 */
#ifndef MORTON_H
#define MORTON_H

#include <stdint.h>

/*
 * Spread the low 16 bits of x so each bit sits at every other position:
 * bit i of the input lands at bit 2*i of the result. Each stage doubles
 * the gap between occupied bits; the mask keeps exactly the bits that
 * do not overlap between the two operands, so XOR and OR coincide
 * everywhere the mask keeps.
 */
static inline uint32_t morton_spread16(uint32_t x)
{
    x = (x ^ (x << 8)) & 0x00FF00FFu;
    x = (x ^ (x << 4)) & 0x0F0F0F0Fu;
    x = (x ^ (x << 2)) & 0x33333333u;
    x = (x ^ (x << 1)) & 0x55555555u;
    return x;
}

/*
 * Compact: the exact inverse of morton_spread16. The low 16 bits of the
 * result are the bits from the even positions 0, 2, ..., 30 of the
 * input. No overlap between the operands at any stage, so XOR equals OR
 * everywhere the mask keeps.
 */
static inline uint32_t morton_compact16(uint32_t x)
{
    x &= 0x55555555u;
    x = (x ^ (x >> 1)) & 0x33333333u;
    x = (x ^ (x >> 2)) & 0x0F0F0F0Fu;
    x = (x ^ (x >> 4)) & 0x00FF00FFu;
    x = (x ^ (x >> 8)) & 0x0000FFFFu;
    return x;
}

static inline uint32_t morton_interleave(uint16_t lo, uint16_t hi)
{
    return morton_spread16(lo) | (morton_spread16(hi) << 1);
}

static inline uint32_t morton_deinterleave(uint32_t z)
{
    return (morton_compact16(z >> 1) << 16) | morton_compact16(z);
}

#endif /* MORTON_H */
