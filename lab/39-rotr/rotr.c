#include "rotr.h"

/*
 * Rotate right: bit p moves to position (p - r) mod 64. The bits that
 * fall off the bottom reappear at the top, which is exactly what
 * (x >> r) OR (x << (64 - r)) computes: the right shift places the high
 * bits, the left shift wraps the low r bits into the top r positions.
 * When r = 0, the left shift amount is (64 - 0) & 63 = 0, so the identity
 * still holds with no undefined shift.
 */
uint64_t rotr64(uint64_t x, unsigned r)
{
	r &= 63u;
	return (x >> r) | (x << ((64u - r) & 63u));
}

/*
 * Rotate left: bit p moves to position (p + r) mod 64, the mirror image
 * of rotr64, so rotr64(rotl64(x, r), r) == x. Built from scratch with the
 * same shift/OR construction; it does not call rotr64 and wraps no
 * library routine.
 */
uint64_t rotl64(uint64_t x, unsigned r)
{
	r &= 63u;
	return (x << r) | (x >> ((64u - r) & 63u));
}
