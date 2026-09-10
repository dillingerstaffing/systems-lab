/* lab/53-double-dabble: 8-bit binary to 3-digit packed BCD via the
 * shift-and-add-3 identity.
 *
 * Packed BCD layout of the result: bits 11..8 are the hundreds digit,
 * bits 7..4 the tens digit, bits 3..0 the ones digit; every digit is in
 * 0..9. Example: dabble_to_bcd(255) returns 0x255, dabble_to_bcd(7)
 * returns 0x007.
 *
 * Construction: one 32-bit shift register. Bits 19..8 are the BCD
 * scratch area (three nibbles), bits 7..0 hold the binary input still
 * to be folded in. For each of the 8 input bits, most significant
 * first:
 *
 *   1. Add-3 pass: for each BCD nibble holding digit d, if d >= 5 add 3.
 *      The identity: doubling digit d gives 2d, and when d >= 5 the
 *      doubled digit needs a decimal carry, since 2d >= 10. Adding 3
 *      moves d into 8..12, and shifting that left one gives low nibble
 *      (2d + 6) mod 16 = 2d - 10 with the shifted-out bit equal to 1
 *      exactly when 2d >= 10. The add-3 therefore pre-arranges the
 *      shift's carry-out to be the decimal carry, and the low nibble
 *      lands on the doubled digit. Digits below 5 double in place with
 *      no carry, so they need no fixup.
 *   2. Shift the whole register left by 1. This doubles the BCD value
 *      held in the scratch area and moves the next binary bit into the
 *      ones position.
 *
 * By induction the scratch area always holds the valid packed BCD of
 * the value of the input bits shifted in so far; after 8 shifts it
 * holds the BCD of the full input. No library conversion, no lookup
 * tables: only shifts, masks, adds, and comparisons on unsigned types.
 * The top bit shifts out into the void; unsigned left shift into the
 * discarded region is well defined.
 *
 * Contract: input is any uint8_t. The returned 12-bit value always has
 * each nibble in 0..9.
 */
#ifndef LAB53_DABBLE_H
#define LAB53_DABBLE_H

#include <stdint.h>

static inline uint16_t dabble_to_bcd(uint8_t value)
{
    uint32_t reg = (uint32_t)value; /* BCD scratch in bits 19..8, input in 7..0 */
    for (int i = 0; i < 8; i++) {
        /* Add-3 pass over the three BCD nibbles: ones = bits 11..8,
         * tens = bits 15..12, hundreds = bits 19..16. */
        if (((reg >> 8) & 0x0F) >= 5)
            reg += (uint32_t)3 << 8;
        if (((reg >> 12) & 0x0F) >= 5)
            reg += (uint32_t)3 << 12;
        if (((reg >> 16) & 0x0F) >= 5)
            reg += (uint32_t)3 << 16;
        reg <<= 1;
    }
    return (uint16_t)((reg >> 8) & 0x0FFF);
}

#endif /* LAB53_DABBLE_H */
