#ifndef BITPERM_H
#define BITPERM_H

#include <stdint.h>

/* Fixed 8-bit bit permutation P and its explicit inverse Q.
 *
 * P sends input bit i to output bit P[i], with
 *     P = [2, 5, 0, 7, 1, 6, 3, 4].
 * The implementation is the unrolled shift/mask sum
 *     out = OR_i (((b >> i) & 1) << P[i]),
 * so every output bit is named explicitly and no lookup table, branch,
 * or loop carries the permutation at run time.
 *
 * Q is the exact index reversal of P: Q[j] is the i with P[i] == j,
 *     Q = [2, 4, 0, 6, 7, 1, 5, 3].
 * Q[P[i]] = i and P[Q[i]] = i for every i, so applying Q after P
 * returns every bit to its original place. The mapping table is
 * re-checked numerically in the test harness (each of 0..7 appears
 * exactly once as a P[i], and Q inverts P index by index).
 *
 * perm16/invperm16 apply the byte permutation to both bytes of a
 * 16-bit word; perm64/invperm64 apply it to all eight bytes of a
 * 64-bit word.
 */

static inline uint8_t perm8(uint8_t b)
{
    return (uint8_t)(
        (((b >> 0) & 1u) << 2) |
        (((b >> 1) & 1u) << 5) |
        (((b >> 2) & 1u) << 0) |
        (((b >> 3) & 1u) << 7) |
        (((b >> 4) & 1u) << 1) |
        (((b >> 5) & 1u) << 6) |
        (((b >> 6) & 1u) << 3) |
        (((b >> 7) & 1u) << 4));
}

static inline uint8_t invperm8(uint8_t b)
{
    return (uint8_t)(
        (((b >> 0) & 1u) << 2) |
        (((b >> 1) & 1u) << 4) |
        (((b >> 2) & 1u) << 0) |
        (((b >> 3) & 1u) << 6) |
        (((b >> 4) & 1u) << 7) |
        (((b >> 5) & 1u) << 1) |
        (((b >> 6) & 1u) << 5) |
        (((b >> 7) & 1u) << 3));
}

static inline uint16_t perm16(uint16_t x)
{
    return (uint16_t)((uint16_t)perm8((uint8_t)(x & 0xFFu)) |
                      ((uint16_t)perm8((uint8_t)(x >> 8)) << 8));
}

static inline uint16_t invperm16(uint16_t x)
{
    return (uint16_t)((uint16_t)invperm8((uint8_t)(x & 0xFFu)) |
                      ((uint16_t)invperm8((uint8_t)(x >> 8)) << 8));
}

static inline uint64_t perm64(uint64_t x)
{
    uint64_t y = 0;
    int i;
    for (i = 0; i < 8; i++)
        y |= (uint64_t)perm8((uint8_t)(x >> (8 * i))) << (8 * i);
    return y;
}

static inline uint64_t invperm64(uint64_t x)
{
    uint64_t y = 0;
    int i;
    for (i = 0; i < 8; i++)
        y |= (uint64_t)invperm8((uint8_t)(x >> (8 * i))) << (8 * i);
    return y;
}

#endif
