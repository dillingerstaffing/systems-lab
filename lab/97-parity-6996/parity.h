#ifndef PARITY_6996_H
#define PARITY_6996_H

#include <stdint.h>

/*
 * parity32: returns 1 if x has an odd number of set bits, else 0.
 *
 * Construction, from two facts about XOR:
 * 1. Parity is the XOR of all 32 bits. XOR is associative and
 *    commutative, so folding halves (x ^= x >> k) moves the XOR of
 *    all bits into the surviving half. After x ^= x >> 16,
 *    x ^= x >> 8, x ^= x >> 4, the low nibble holds the XOR of all
 *    32 bits, i.e. the parity of the original word.
 * 2. The parity of a 4-bit value i is bit i of the constant 0x6996,
 *    verified by the hand derivation in PROOF.md. Hence
 *    (0x6996u >> (x & 0xf)) & 1u reads the parity of the folded
 *    nibble, which is the parity of the original word.
 *
 * No library popcount or parity builtin is used.
 */
int parity32(uint32_t x);

#endif
