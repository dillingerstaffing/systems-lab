#include "gray.h"

/*
 * Encode: bit k of the Gray code is n_k XOR n_{k+1} (with n_16 = 0).
 * XORing every adjacent pair at once is exactly n ^ (n >> 1).
 */
uint16_t gray16_encode(uint16_t n)
{
	return (uint16_t)(n ^ (n >> 1));
}

/*
 * Decode: binary bit k is g_k XOR b_{k+1} (with b_16 = 0). Solving the
 * chain of XOR equations top-down gives the xor-fold: each fold step
 * propagates the already-decoded high bits into the low half, and four
 * steps (8, 4, 2, 1) cover all 16 bit positions.
 */
uint16_t gray16_decode(uint16_t g)
{
	g ^= (uint16_t)(g >> 8);
	g ^= (uint16_t)(g >> 4);
	g ^= (uint16_t)(g >> 2);
	g ^= (uint16_t)(g >> 1);
	return g;
}
