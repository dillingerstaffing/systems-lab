/*
 * clz_byte.c - build the 8-bit leading-zero table from the bit-by-bit
 * identity.
 */
#include "clz_byte.h"

uint8_t clz8_tab[256];

void clz8_build(void)
{
    for (int b = 0; b < 256; b++) {
        unsigned v = (unsigned)b;
        int n = 0;
        /* Shift left until a 1 reaches bit 7; the shift count is the
         * number of leading zeros of this byte value.  For b == 0 no 1
         * ever arrives, so n reaches 8: a zero byte has 8 leading
         * zeros. */
        while (n < 8 && (v & 0x80U) == 0U) {
            n++;
            v <<= 1;
        }
        clz8_tab[b] = (uint8_t)n;
    }
}
