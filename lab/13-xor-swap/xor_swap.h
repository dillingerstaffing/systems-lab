#ifndef XOR_SWAP_H
#define XOR_SWAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * xor_swap: swap the 64-bit words at *a and *b in place, using only the
 * xor group identities, no temporary variable, no builtins:
 *
 *     *a ^= *b;
 *     *b ^= *a;
 *     *a ^= *b;
 *
 * Why it works: xor is associative, x ^ x == 0, and x ^ 0 == x for every
 * 64-bit word x. Writing the three steps as assignments over the old
 * values (a0, b0):
 *     a1 = a0 ^ b0
 *     b1 = a1 ^ a0 = (a0 ^ b0) ^ a0 = b0
 *     a2 = a1 ^ b1 = (a0 ^ b0) ^ b0 = a0
 * so *a ends as b0 and *b ends as a0.
 *
 * Aliasing contract: if a == b (the same address), the three steps
 * collapse onto one word: v ^= v makes it 0 and the remaining steps keep
 * it 0. So xor_swap(&x, &x) zeroes x. Callers that need identity when the
 * pointers are equal must guard before calling.
 */
void xor_swap(uint64_t *a, uint64_t *b);

#ifdef __cplusplus
}
#endif

#endif
