#ifndef CRC16_CCITT_H
#define CRC16_CCITT_H

#include <stddef.h>
#include <stdint.h>

/*
 * crc16_ccitt(data, len): the 16-bit CRC with polynomial x^16 + x^12 + x^5 + 1
 * (0x1021), computed in reflected form. The polynomial bit-reversed is
 * 0x8408, so each byte is folded into the register least-significant-bit
 * first: for every bit, the register shifts right one and, if the bit
 * shifted out was 1, is xored with 0x8408. That shift/xor recurrence is the
 * whole implementation: no table, no library call, no builtin.
 *
 * Parameters: init 0x0000, no reflection is needed on top of the reflected
 * loop, xorout 0x0000. This matches the CRC catalogue's CRC-16/KERMIT
 * entry (poly 0x1021, refin true, refout true, init 0x0000, xorout 0x0000),
 * whose check value for "123456789" is 0x2189.
 */
static inline uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (uint16_t)((crc & 1u) ? ((crc >> 1) ^ 0x8408u)
                                        : (crc >> 1));
    }
    return crc;
}

#endif
