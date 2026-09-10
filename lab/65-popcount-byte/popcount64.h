/*
 * popcount64.h - population count of a 64-bit word via a 256-entry
 * byte table.
 *
 * Two identities do all the work, and both are exact, not approximate:
 *
 * 1. One-bit identity.  popcount(i) for an 8-bit i equals the low bit
 *    plus the popcount of i shifted right one: popcount(i) = (i & 1) +
 *    popcount(i >> 1).  The table is built from this identity alone,
 *    with table[0] = 0.  No literal popcounts are hard-coded anywhere.
 *
 * 2. Byte-decomposition sum identity.  The set bits of a 64-bit word
 *    are partitioned by the eight bytes, so the word's popcount is the
 *    sum of the eight byte popcounts: popcount(x) =
 *    table[(x >> 8k) & 0xFF] summed for k = 0..7.
 *
 * popcount_table_init() must be called once before popcount64().
 * All arithmetic is unsigned; no shifts reach the word width; no UB.
 */
#ifndef POPCOUNT64_H
#define POPCOUNT64_H

#include <stdint.h>

static unsigned char popcount_table[256];

static void popcount_table_init(void)
{
    popcount_table[0] = 0;
    for (int i = 1; i < 256; i++)
        popcount_table[i] = (unsigned char)((i & 1) + popcount_table[i >> 1]);
}

static unsigned int popcount64(uint64_t x)
{
    unsigned int n = 0;
    for (int k = 0; k < 8; k++)
        n += popcount_table[(x >> (8 * k)) & 0xFFU];
    return n;
}

#endif /* POPCOUNT64_H */
