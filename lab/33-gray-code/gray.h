#ifndef GRAY_H
#define GRAY_H

#include <stdint.h>

/*
 * Gray code encode/decode for 16-bit values.
 *
 * gray16_encode(n): returns n ^ (n >> 1).
 *
 * gray16_decode(g): inverts gray16_encode by xor-folding:
 *   g ^= g >> 8; g ^= g >> 4; g ^= g >> 2; g ^= g >> 1; return g.
 *
 * The two are inverses: gray16_decode(gray16_encode(x)) == x for every
 * 16-bit x. Successive values differ in exactly one bit:
 * popcount(gray16_encode(x) ^ gray16_encode(x + 1)) == 1.
 */
uint16_t gray16_encode(uint16_t n);
uint16_t gray16_decode(uint16_t g);

#endif
