#include "parity.h"

/*
 * parity32 via xor-fold to one nibble, then the 0x6996 nibble-parity
 * table constant.
 *
 * Fact A (fold): parity(x) is the XOR of all 32 bits. Write x as two
 * 16-bit halves hi:lo. After x ^= x >> 16, the low half is hi ^ lo,
 * and parity(hi ^ lo) = parity(hi) ^ parity(lo) = parity(x), because
 * the XOR of all bits is what survives bitwise XOR of the halves.
 * Repeating for k = 8 and k = 4 puts the XOR of all 32 bits into the
 * low nibble; the word's parity then equals the low nibble's parity.
 *
 * Fact B (constant): bit i of 0x6996 equals the parity of the 4-bit
 * value i, for every i in 0..15. The full hand derivation of the bit
 * layout is in PROOF.md; the table is not a lookup into unknown data,
 * each bit is checked against the naive definition there.
 *
 * Together: (0x6996u >> (folded x & 0xf)) & 1u is 1 exactly when the
 * original word has an odd number of set bits.
 */
int parity32(uint32_t x)
{
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    return (int)((0x6996u >> (x & 0xfu)) & 1u);
}
