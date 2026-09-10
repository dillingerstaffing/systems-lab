#ifndef BITMATRIX_TRANSPOSE_H
#define BITMATRIX_TRANSPOSE_H

#include <stdint.h>

/*
 * bitmatrix_transpose64: transpose of an 8x8 bit matrix packed in a
 * uint64_t.  Bit (r, c) lives at linear index 8*r + c; the transpose
 * moves it to index 8*c + r.
 *
 * Write the 6-bit index as r2 r1 r0 c2 c1 c0 (bits 5..0).  The transpose
 * swaps the two 3-bit halves, which factors into three swaps of single
 * index bits: (bit0, bit3), (bit1, bit4), (bit2, bit5).
 *
 * Swapping index bits (a, b) with a < b pairs up positions at linear
 * distance 2^b - 2^a: flipping bit a from 1 to 0 subtracts 2^a and
 * flipping bit b from 0 to 1 adds 2^b.  Hence the three stages are
 * conditional swaps at distances 7 = 8-1, 14 = 16-2, and 28 = 32-4.
 *
 * Each stage is a delta swap: for distance d and mask m, bit i of m set
 * means "exchange positions i and i+d":
 *
 *     t = (x ^ (x >> d)) & m;  x ^= t ^ (t << d);
 *
 * The mask holds exactly the lower member of every pair: positions with
 * bit a = 1 and bit b = 0.  Every position is either fixed (bits a and b
 * equal) or in exactly one pair, so the three stages compose to the
 * transpose.  The stages touch disjoint index-bit pairs, so they commute;
 * any order gives the same result.
 *
 * Masks (derived from the rule above, verified by exhaustive
 * differential test in test_bitmatrix_transpose.c):
 *   d=7  (bits 0,3): 0x00AA00AA00AA00AA
 *   d=14 (bits 1,4): 0x0000CCCC0000CCCC
 *   d=28 (bits 2,5): 0x00000000F0F0F0F0
 *
 * All operations are unsigned 64-bit shifts, xors, and ands: no tables,
 * no builtins, no library calls, no undefined behavior.  The largest
 * set bit of any mask plus its stage distance stays below 64
 * (55+7=62, 46+14=60, 31+28=59), so every shift is in range.
 */

#define BM_T_MASK7  0x00AA00AA00AA00AAULL
#define BM_T_MASK14 0x0000CCCC0000CCCCULL
#define BM_T_MASK28 0x00000000F0F0F0F0ULL

static inline uint64_t bm_t_delta_swap(uint64_t x, unsigned d, uint64_t m)
{
    uint64_t t = (x ^ (x >> d)) & m;
    return x ^ t ^ (t << d);
}

static inline uint64_t bitmatrix_transpose64(uint64_t x)
{
    x = bm_t_delta_swap(x, 7, BM_T_MASK7);
    x = bm_t_delta_swap(x, 14, BM_T_MASK14);
    x = bm_t_delta_swap(x, 28, BM_T_MASK28);
    return x;
}

#endif /* BITMATRIX_TRANSPOSE_H */
