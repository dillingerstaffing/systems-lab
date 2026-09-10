#ifndef CRC16_TABLE_H
#define CRC16_TABLE_H

#include <stddef.h>
#include <stdint.h>

/*
 * CRC-16 with polynomial x^16 + x^12 + x^5 + 1 (0x1021), computed
 * most-significant-bit first: the register starts at 0xFFFF, each byte is
 * folded into the top 8 bits of the register, and every one of its 8 bits
 * shifts the register left one place, xoring 0x1021 when the bit shifted
 * out of the top is 1. Nothing is reflected, nothing is xored out.
 * This is the parameter set the CRC catalogue lists as CRC-16/CCITT-FALSE
 * (poly 0x1021, refin false, refout false, init 0xFFFF, xorout 0x0000),
 * whose check value for "123456789" is 0x29B1.
 */

/* One-bit-at-a-time shift-register recurrence. The whole algorithm is the
 * loop below: no table, no library call, no builtin. */
static inline uint16_t crc16_bitwise(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);
        for (int b = 0; b < 8; b++)
            crc = (uint16_t)((crc & 0x8000u) ? ((unsigned)(crc << 1) ^ 0x1021u)
                                             : (unsigned)(crc << 1));
    }
    return crc;
}

/*
 * Table-driven version. crc16_table_state[i] is the CRC of the single byte
 * i produced by the same shift/xor recurrence above, starting from a zero
 * register with the byte in the top 8 bits; crc16_table_init() derives all
 * 256 entries from the polynomial at startup, there is no hard-coded table
 * anywhere in this file. The per-byte update is
 *     crc = (crc << 8) ^ table[((crc >> 8) ^ byte) & 0xFF],
 * which processes the same division one byte at a time instead of one bit.
 */
static uint16_t crc16_table_state[256];
static int crc16_table_ready = 0;

static void crc16_table_init(void)
{
    for (int i = 0; i < 256; i++) {
        uint16_t crc = (uint16_t)(i << 8);
        for (int b = 0; b < 8; b++)
            crc = (uint16_t)((crc & 0x8000u) ? ((unsigned)(crc << 1) ^ 0x1021u)
                                             : (unsigned)(crc << 1));
        crc16_table_state[i] = crc;
    }
    crc16_table_ready = 1;
}

static inline uint16_t crc16_table(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; i++)
        crc = (uint16_t)((crc << 8) ^
                         crc16_table_state[((crc >> 8) ^ data[i]) & 0xFFu]);
    return crc;
}

#endif
