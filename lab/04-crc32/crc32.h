#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>

/*
 * CRC32 (IEEE 802.3) in two independent implementations:
 *
 *   crc32_bitwise  - direct transcription of polynomial long division
 *                    over GF(2), one bit at a time.
 *   crc32_table    - same math, 8 bits at a time via a 256-entry table
 *                    derived from the generator polynomial at startup.
 *
 * Parameters for both: generator polynomial 0x04C11DB7 (reflected form
 * 0xEDB88320), register init 0xFFFFFFFF, final xor 0xFFFFFFFF,
 * input and output reflected.
 */
uint32_t crc32_bitwise(const void *data, size_t len);
uint32_t crc32_table(const void *data, size_t len);

/* Byte-swapped check value helper used by the test only. */
uint32_t crc32_reflected_check(void);

#endif
