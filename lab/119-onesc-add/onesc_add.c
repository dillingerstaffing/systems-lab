#include "onesc_add.h"

/*
 * End-around carry, recovered from the wraparound identity alone:
 * in 16-bit wraparound arithmetic, (uint16_t)(a + b) < a if and
 * only if the true 17-bit sum overflowed, so the comparison yields
 * the carry as a 0/1 value without touching the checksum loop or
 * any wider accumulator.
 */
uint16_t onesc_add(uint16_t a, uint16_t b)
{
    uint16_t s = (uint16_t)(a + b);
    uint16_t carry = (uint16_t)(s < a);
    return (uint16_t)(s + carry);
}
