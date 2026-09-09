#include "parity.h"

/*
 * parity64 via the xor-fold reduction.
 *
 * Why the fold is valid: write x as hi:lo, two k-bit halves. The XOR
 * of all 2k bits equals parity(hi) ^ parity(lo), and after
 * x ^= x >> k the low k bits hold hi ^ lo, whose parity is again
 * parity(hi) ^ parity(lo). So each fold preserves the XOR of all
 * bits in the surviving half. Folding k = 32, 16, 8, 4, 2, 1 leaves
 * the XOR of the original 64 bits in bit 0; masking isolates it.
 *
 * parity64(x) == 1 exactly when x has an odd number of set bits,
 * because XOR of the bits is addition modulo 2.
 */
int parity64(uint64_t x)
{
    x ^= x >> 32;
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return (int)(x & 1u);
}
