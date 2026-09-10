#include "bcd.h"

uint8_t bcd_invalid_mask(uint16_t w)
{
    /*
     * Two masked adds, lanes spaced 8 bits apart with zero bits in both
     * addends between lanes. Proof sketch (full bit-by-bit proof in
     * PROOF.md):
     *
     * 1. Add-guard: for nibble n in 0..15, n + 6 >= 16 iff n >= 10, so a
     *    carry out of a lane happens exactly for an invalid nibble, once
     *    the carry into the lane is known to be 0.
     * 2. Lane isolation: in the even add, bits 4..7 are 0 in both
     *    addends; a full adder with both addend bits 0 outputs carry 0,
     *    so carries out of nibble 0 die at bit 4 and the carry into
     *    nibble 2's lane (bit 8) is 0. Same argument for the odd add
     *    (bits 0..3 and 8..11 zeroed), giving carry 0 into bits 4 and 12.
     * 3. Extraction: carry into bit k of t = a + b is
     *    c_k = t_k ^ a_k ^ b_k. At each lane boundary the two addend
     *    bits are 0 (checked from the constants below), so the generated
     *    carry is exactly the sum bit at the boundary.
     */
    uint32_t t_even = (uint32_t)(w & 0x0F0Fu) + 0x0606u;  /* lanes: nibbles 0, 2 */
    uint32_t t_odd  = (uint32_t)(w & 0xF0F0u) + 0x6060u;  /* lanes: nibbles 1, 3 */

    uint8_t m = 0;
    m |= (uint8_t)(((t_even >> 4)  & 1u) << 0);  /* nibble 0 carry out */
    m |= (uint8_t)(((t_odd  >> 8)  & 1u) << 1);  /* nibble 1 carry out */
    m |= (uint8_t)(((t_even >> 12) & 1u) << 2);  /* nibble 2 carry out */
    m |= (uint8_t)(((t_odd  >> 16) & 1u) << 3);  /* nibble 3 carry out */
    return m;
}
