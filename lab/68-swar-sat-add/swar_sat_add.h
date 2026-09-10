/*
 * swar_sat_add.h - saturating add of four packed 16-bit lanes in one
 * 64-bit word.
 *
 * Input: two uint64_t words, each holding four unsigned 16-bit lanes in
 * bits [0:16), [16:32), [32:48), [48:64).
 * Output: per-lane sum, clamped to 0xFFFF when a lane overflows.
 *
 * The 64-bit add cannot be used directly: a carry out of one lane would
 * contaminate the next lane.  Instead, even lanes (0, 2) and odd lanes
 * (1, 3) are handled as two independent 32-bit-spaced pairs.  In each
 * pair the lanes sit 32 bits apart, so a lane sum (at most 0x1FFFE,
 * 17 bits) has its carry-out land on a guard bit (bit 16 / bit 48 of
 * the pair) that belongs to no lane.  The guard bits are read back as
 * overflow flags, broadcast to full lane masks by multiplying with
 * 0xFFFF, and select between the raw lane sum and 0xFFFF, branchless.
 *
 * Odd lanes are shifted down by 16 first so lane 3 (the top lane, whose
 * carry-out would otherwise be lost off the top of the word) gets a
 * guard bit too, then shifted back into place.
 */
#ifndef SWAR_SAT_ADD_H
#define SWAR_SAT_ADD_H

#include <stdint.h>

static inline uint64_t swar_sat_add16x4(uint64_t a, uint64_t b)
{
    /* Lane positions for one parity: bits [0:16) and [32:48). */
    const uint64_t E = 0x0000FFFF0000FFFFULL;
    /* Guard-bit positions after adding the masked pairs. */
    const uint64_t G = 0x000100000001ULL;

    /* Even lanes 0 and 2.  Lane sums land in [0:16)/[32:48), carries
     * from 16-bit overflow land on the guard bits 16 and 48. */
    uint64_t e = (a & E) + (b & E);
    uint64_t eov = ((e >> 16) & G) * 0xFFFFULL;
    uint64_t re = ((e & E) & ~eov) | eov;

    /* Odd lanes 1 and 3, shifted down into the same E positions so
     * lane 3 also gets a guard bit, then shifted back up. */
    uint64_t o = ((a >> 16) & E) + ((b >> 16) & E);
    uint64_t oov = ((o >> 16) & G) * 0xFFFFULL;
    uint64_t ro = (((o & E) & ~oov) | oov) << 16;

    return re | ro;
}

#endif /* SWAR_SAT_ADD_H */
