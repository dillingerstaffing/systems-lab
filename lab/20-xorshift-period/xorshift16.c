#include "xorshift16.h"

/*
 * One step of the (7, 9, 8) 16-bit xorshift recurrence.
 *
 * C's integer promotions lift the uint16_t operand to int before
 * each shift, so the explicit (uint16_t) narrowing after every line
 * reproduces exact 16-bit register behavior: bits shifted past bit 15
 * are discarded, and every shift count is below the 16-bit width, so
 * no shift is ever undefined. All arithmetic is on unsigned types.
 */
uint16_t xorshift16_step(uint16_t x)
{
    x ^= (uint16_t)((uint32_t)x << 7);
    x ^= (uint16_t)((uint32_t)x >> 9);
    x ^= (uint16_t)((uint32_t)x << 8);
    return x;
}
