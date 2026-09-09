#ifndef PARITY_H
#define PARITY_H

#include <stdint.h>

/*
 * parity64: returns 1 if x has an odd number of set bits, else 0.
 *
 * The implementation in parity.c is built only from the xor-fold
 * reduction: parity of a word equals the XOR of all its bits, and
 * XOR is associative and commutative, so folding the high half into
 * the low half (x ^= x >> k) leaves the XOR of all bits unchanged.
 * Repeatedly halving k until one bit remains leaves that answer in
 * bit 0. No library popcount or parity builtin is used.
 */
int parity64(uint64_t x);

#endif
