#include "hamming.h"

/* popcount64 via the SWAR parallel-add identities (the same construction
 * as lab/17-bitcount, rebuilt here so this module stands alone).
 *
 * The identity behind each step: adding two 1-bit counters packed in a
 * field of n bits stays inside the field, so one subtraction/addition
 * adds all packed counters at once, with no carry leaking into the
 * neighboring field. The masks enforce the field boundaries.
 *
 *   x - ((x >> 1) & 0x5555...)
 *       adds each adjacent pair of bits into a 2-bit counter (0..2).
 *   (x & 0x3333...) + ((x >> 2) & 0x3333...)
 *       adds each pair of 2-bit counters into a 4-bit counter (0..4).
 *   (x + (x >> 4)) & 0x0F0F...
 *       adds each pair of 4-bit counters into an 8-bit counter (0..8).
 *       The mask applies after the add; the only carries that can
 *       cross a field boundary are the low bits of a field adding the
 *       field's high nibble into the next field's low nibble, and the
 *       mask discards exactly those stray bits (each field holds at
 *       most 8, so nothing real is lost).
 *   (x * 0x0101010101010101) >> 56
 *       horizontal fold: each byte is a value 0..8, and the multiply
 *       sums all eight bytes into the top byte because each byte's
 *       weight (1, 256, 65536, ...) distributes the 8 byte values into
 *       disjoint bit ranges; the sum of eight bytes each <= 8 fits in
 *       the top byte with room to spare (max 64 < 256), so the top
 *       byte after the multiply is exactly the total.
 */
static uint32_t popcount64(uint64_t x)
{
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (uint32_t)((x * 0x0101010101010101ULL) >> 56);
}

/* hamming64(a, b): the identity is d(a, b) = popcount(a ^ b).
 * a ^ b has exactly the bits set where a and b differ, and nothing
 * else; counting those bits is the Hamming distance. No library
 * popcount is wrapped: the count comes from the SWAR identities
 * above, in this same translation unit.
 */
uint32_t hamming64(uint64_t a, uint64_t b)
{
    return popcount64(a ^ b);
}
