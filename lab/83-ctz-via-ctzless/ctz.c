#include "ctz.h"

/* popcount built from the SWAR parallel-add identities. The argument
 * for each step: adding packed counters inside a field of bits stays
 * inside the field, so one addition sums all fields at once and the
 * masks cut away the only stray carries (bits that leak into the next
 * field's low positions, which the mask then discards).
 *
 *   x - ((x >> 1) & 0x5555...):  each adjacent pair of bits becomes a
 *       2-bit counter holding 0..2.
 *   (x & 0x3333...) + ((x >> 2) & 0x3333...):  each pair of 2-bit
 *       counters becomes a 4-bit counter holding 0..4.
 *   (x + (x >> 4)) & 0x0F0F...:  each pair of 4-bit counters becomes an
 *       8-bit counter holding 0..8 (the mask removes the one possible
 *       cross-field carry nibble, which carries no real count).
 *   (x * 0x0101010101010101) >> 56:  horizontal fold; each byte is a
 *       value 0..8, the multiply sums the eight bytes into the top
 *       byte because eight bytes each <= 8 sum to at most 64 < 256.
 */
static uint32_t swar_popcount64(uint64_t x)
{
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (uint32_t)((x * 0x0101010101010101ULL) >> 56);
}

uint32_t ctz64_identity(uint64_t x)
{
    /* The stated identity, nothing else. */
    return swar_popcount64((x ^ (x - 1)) >> 1);
}
