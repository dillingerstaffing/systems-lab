#ifndef CRC32C_H
#define CRC32C_H

#include <stddef.h>
#include <stdint.h>

/* CRC-32C (Castagnoli), reflected, init 0xFFFFFFFF, xorout 0xFFFFFFFF. */
/* Bitwise GF(2) long-division reference: 8 conditional steps per byte. */
uint32_t crc32c_bitwise(const uint8_t *data, size_t len);

/* Build the 256-entry table from the generator polynomial. Must be called
 * before crc32c_table or crc32c_table_check. */
void crc32c_table_init(void);

/* Re-derive every table entry straight from the polynomial and report the
 * first index that disagrees, or -1 when all 256 entries check out. */
int crc32c_table_check(void);

/* Table-driven variant, one lookup per byte. */
uint32_t crc32c_table(const uint8_t *data, size_t len);

#endif
