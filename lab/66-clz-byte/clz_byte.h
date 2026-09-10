/*
 * clz_byte.h - count leading zeros of a 64-bit word from an 8-bit table
 * plus the byte-scan identity.
 *
 * Contract: clz64_byte(0) == 64.
 *
 * For x != 0, let B be the most significant nonzero byte of x (byte 7
 * holds bits [56:64)).  Every bit above B is zero: 8*(7-index(B)) of
 * them.  B's own leading zeros within its 8 bits are clz8_tab[B].  So
 * clz64(x) = 8*(7-index(B)) + clz8_tab[B].  The scan below finds B; the
 * table holds each byte value's leading-zero count.
 *
 * The table is filled by clz8_build(): for each byte value, shift left
 * until a 1 reaches bit 7 (or 8 shifts pass), counting the shifts.
 * That count is the byte's leading-zero count by the definition of the
 * operation itself, so the table carries no assumed data.
 */
#ifndef CLZ_BYTE_H
#define CLZ_BYTE_H

#include <stdint.h>

/* 256-entry table: clz8_tab[b] = leading zeros of the 8-bit value b. */
extern uint8_t clz8_tab[256];

/* Fill clz8_tab[] from the bit-by-bit identity.  Call once before use. */
void clz8_build(void);

/* Count leading zeros of x; returns 64 for x == 0. */
static inline uint64_t clz64_byte(uint64_t x)
{
    for (int i = 7; i >= 0; i--) {
        uint8_t b = (uint8_t)(x >> (8 * i));
        if (b != 0)
            return (uint64_t)(8 * (7 - i)) + clz8_tab[b];
    }
    return 64;
}

#endif /* CLZ_BYTE_H */
