#ifndef ONESC_ADD_H
#define ONESC_ADD_H

#include <stdint.h>

/*
 * onesc_add: 16-bit one's-complement addition.
 *
 * The wraparound carry is folded back in (end-around carry): the
 * sum is computed in 16-bit wraparound arithmetic, the single lost
 * carry bit is recovered from the wraparound identity (s < a can
 * only happen when the addition wrapped, so the comparison is the
 * 0/1 carry), and the carry is added back into the low 16 bits.
 *
 * 0xFFFF + 0xFFFF yields 0xFFFF, not zero: the identity must hold
 * on the all-ones inputs that represent one's-complement zero.
 */
uint16_t onesc_add(uint16_t a, uint16_t b);

#endif
