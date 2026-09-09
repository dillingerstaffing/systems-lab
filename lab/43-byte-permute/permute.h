/* Byte permutation on 64-bit words by shift/mask composition.
 *
 * permute64 applies one fixed non-identity permutation of the 8 bytes of a
 * 64-bit word. Output byte i is taken from input byte P[i], where
 * P = (2,5,0,7,1,6,3,4). Each byte moves as an indivisible 8-bit unit:
 *
 *   out |= ((x >> (8*src)) & 0xFF) << (8*dst)
 *
 * which is the direct consequence of the two identities
 *   (x >> (8*k)) & 0xFF  ==  byte k of x
 *   b << (8*k)           ==  b placed as byte k of a word
 *
 * inv_permute64 applies the exact inverse permutation, derived from P by
 * reading it backwards: inv[P[i]] = i gives INV = (2,4,0,6,7,1,5,3).
 * Both functions are written as explicit compositions with hardcoded
 * source indices; no lookup tables anywhere in this header.
 */
#ifndef BYTE_PERMUTE_H
#define BYTE_PERMUTE_H

#include <stdint.h>

/* Output byte i = input byte (2,5,0,7,1,6,3,4)[i]. */
static inline uint64_t permute64(uint64_t x)
{
    return (((x >> (8u * 2)) & 0xFFu) << (8u * 0)) |
           (((x >> (8u * 5)) & 0xFFu) << (8u * 1)) |
           (((x >> (8u * 0)) & 0xFFu) << (8u * 2)) |
           (((x >> (8u * 7)) & 0xFFu) << (8u * 3)) |
           (((x >> (8u * 1)) & 0xFFu) << (8u * 4)) |
           (((x >> (8u * 6)) & 0xFFu) << (8u * 5)) |
           (((x >> (8u * 3)) & 0xFFu) << (8u * 6)) |
           (((x >> (8u * 4)) & 0xFFu) << (8u * 7));
}

/* Inverse permutation of P: inv[P[i]] = i, so INV = (2,4,0,6,7,1,5,3). */
static inline uint64_t inv_permute64(uint64_t x)
{
    return (((x >> (8u * 2)) & 0xFFu) << (8u * 0)) |
           (((x >> (8u * 4)) & 0xFFu) << (8u * 1)) |
           (((x >> (8u * 0)) & 0xFFu) << (8u * 2)) |
           (((x >> (8u * 6)) & 0xFFu) << (8u * 3)) |
           (((x >> (8u * 7)) & 0xFFu) << (8u * 4)) |
           (((x >> (8u * 1)) & 0xFFu) << (8u * 5)) |
           (((x >> (8u * 5)) & 0xFFu) << (8u * 6)) |
           (((x >> (8u * 3)) & 0xFFu) << (8u * 7));
}

#endif /* BYTE_PERMUTE_H */
