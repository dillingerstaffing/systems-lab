#include "floor_log2.h"

/*
 * Population count over 64 bits using the parallel (SWAR) reduction.
 * Each line folds adjacent bit-groups into one counter: 2-bit counts
 * into 4-bit groups, then 4-bit into 8-bit, then the byte sums are
 * accumulated into the lowest byte. All arithmetic is unsigned.
 */
static uint64_t popcount64(uint64_t x)
{
    x -= (x >> 1) & 0x5555555555555555u;
    x = (x & 0x3333333333333333u) + ((x >> 2) & 0x3333333333333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Fu;
    x += x >> 8;
    x += x >> 16;
    x += x >> 32;
    return x & 0x7Fu;
}

uint64_t floor_log2(uint64_t x)
{
    /*
     * Propagate the highest set bit down through every lower position.
     * Invariant after all six steps: x has all k + 1 low bits set,
     * where k is the index of the original highest set bit.
     */
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    /*
     * A mask with k + 1 bits set has popcount k + 1, so popcount - 1
     * is k. For x = 0 the propagation is the identity, popcount is 0,
     * and 0 - 1 wraps to UINT64_MAX per the documented convention.
     */
    return popcount64(x) - 1u;
}
