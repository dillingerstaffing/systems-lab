/* lab/44-bcd-add: single-digit packed BCD add and subtract.
 *
 * A packed BCD byte holds two decimal digits: the high nibble is the tens
 * digit and the low nibble the units digit, so the byte 0x42 means 42.
 *
 * Contract: both operands must be valid packed BCD, i.e. each nibble in
 * 0..9. Bytes with a nibble above 9 are outside the contract; behavior
 * for them is undefined and untested.
 *
 * bcd_add(a, b, carry_out): adds two valid packed BCD bytes and returns
 * the low two decimal digits of the sum as packed BCD. *carry_out is 1
 * when a + b >= 100, else 0. Example: bcd_add(0x99, 0x01) returns 0x00
 * with carry 1.
 *
 * bcd_sub(a, b, borrow_out): subtracts two valid packed BCD bytes and
 * returns (a - b) mod 100 as packed BCD. *borrow_out is 1 when a < b,
 * else 0. Example: bcd_sub(0x00, 0x01) returns 0x99 with borrow 1.
 */
#ifndef LAB44_BCD_H
#define LAB44_BCD_H

#include <stdint.h>

static inline uint8_t bcd_add(uint8_t a, uint8_t b, uint8_t *carry_out)
{
    /* Low nibble: t = x + y lies in 0..18. When t > 9 the decimal digit
     * is t - 10 and a carry of 1 propagates to the next nibble. Adding 6
     * instead moves t into 16..24, where bit 4 is exactly that carry and
     * the low nibble is t + 6 - 16 = t - 10. One branch computes both. */
    unsigned t = (unsigned)(a & 0x0F) + (unsigned)(b & 0x0F);
    unsigned c = 0;
    if (t > 9) {
        t += 6;
        c = 1;
    }
    uint8_t lo = (uint8_t)(t & 0x0F);

    /* High nibble: t = x + y + cin lies in 0..19. Same identity; the
     * carry this nibble produces is the out-of-range carry. */
    t = (unsigned)(a >> 4) + (unsigned)(b >> 4) + c;
    c = 0;
    if (t > 9) {
        t += 6;
        c = 1;
    }
    *carry_out = (uint8_t)c;
    return (uint8_t)(((uint8_t)(t & 0x0F) << 4) | lo);
}

static inline uint8_t bcd_sub(uint8_t a, uint8_t b, uint8_t *borrow_out)
{
    /* Low nibble: t = x - y lies in -9..9. When t < 0 the decimal digit
     * is t + 10 and a borrow of 1 propagates. Subtracting 6 instead moves
     * t into -15..-7, where the low nibble of the value is
     * t - 6 + 16 = t + 10. One branch computes both. */
    int t = (int)(a & 0x0F) - (int)(b & 0x0F);
    unsigned bo = 0;
    if (t < 0) {
        t -= 6;
        bo = 1;
    }
    uint8_t lo = (uint8_t)(t & 0x0F);

    /* High nibble: t = x - y - bin lies in -10..9. Same identity; the
     * borrow this nibble produces is the out-of-range borrow. */
    t = (int)(a >> 4) - (int)(b >> 4) - (int)bo;
    bo = 0;
    if (t < 0) {
        t -= 6;
        bo = 1;
    }
    *borrow_out = (uint8_t)bo;
    return (uint8_t)(((uint8_t)(t & 0x0F) << 4) | lo);
}

#endif /* LAB44_BCD_H */
