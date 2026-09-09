#include "endian.h"

/*
 * Byte reversal is a permutation of byte lanes: byte k of the input
 * becomes byte (N-1-k) of the output. A shift by a whole multiple of 8
 * moves a byte from lane to lane exactly, and a mask of 0xFF isolates
 * one lane before the shift, so each shifted piece is disjoint from the
 * others and OR merges them without overlap. Each identity below is a
 * direct transcription of that lane permutation; no builtin, no libc
 * byte-swap, no inline assembly.
 *
 * The 32- and 64-bit forms work in stages: first swap adjacent bytes
 * inside each 2-byte pair, then swap adjacent 2-byte pairs inside each
 * 4-byte group (and, for 64 bits, swap the two 4-byte halves). After
 * all stages, lane k holds the byte that started in lane (N-1-k).
 */

uint16_t u16_swap(uint16_t x)
{
	return (uint16_t)(((x & 0x00FFu) << 8) | ((x & 0xFF00u) >> 8));
}

uint32_t u32_swap(uint32_t x)
{
	x = ((x & 0x00FF00FFu) << 8) | ((x >> 8) & 0x00FF00FFu);
	return (x << 16) | (x >> 16);
}

uint64_t u64_swap(uint64_t x)
{
	x = ((x & 0x00FF00FF00FF00FFull) << 8) | ((x >> 8) & 0x00FF00FF00FF00FFull);
	x = ((x & 0x0000FFFF0000FFFFull) << 16) | ((x >> 16) & 0x0000FFFF0000FFFFull);
	return (x << 32) | (x >> 32);
}
