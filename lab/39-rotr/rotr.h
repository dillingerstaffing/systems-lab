#ifndef ROTR_H
#define ROTR_H

#include <stdint.h>

/*
 * Bit rotation for 64-bit words, built only from the shift/OR identities.
 *
 * rotr64(x, r): (x >> r) | (x << ((64 - r) & 63))
 * rotl64(x, r): (x << r) | (x >> ((64 - r) & 63))
 *
 * The & 63 mask on the complementary shift keeps every shift amount in
 * [0, 63], so r = 0 is well defined (shifting a 64-bit value by 64 is
 * undefined in C; the mask turns it into a shift by 0 of a zero shift).
 * The rotation amount is reduced mod 64 on entry.
 *
 * Invariant: rotr64(rotl64(x, r), r) == x for every 64-bit x and every r.
 */
uint64_t rotr64(uint64_t x, unsigned r);
uint64_t rotl64(uint64_t x, unsigned r);

#endif
