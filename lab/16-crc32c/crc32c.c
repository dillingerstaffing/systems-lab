#include "crc32c.h"

/* Castagnoli generator polynomial, reflected form. The full-degree
 * polynomial is 0x1EDC6F41; reflected (LSB first) it is 0x82F63B78. */
static const uint32_t CRC32C_POLY = 0x82F63B78u;

uint32_t crc32c_bitwise(const uint8_t *data, size_t len) {
	uint32_t crc = 0xFFFFFFFFu;
	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int bit = 0; bit < 8; bit++) {
			if (crc & 1u)
				crc = (crc >> 1) ^ CRC32C_POLY;
			else
				crc >>= 1;
		}
	}
	return crc ^ 0xFFFFFFFFu;
}

/* Direct one-byte derivation of a table entry: the remainder of the
 * single-byte message when divided by the generator polynomial, shifted
 * into the top byte to match the slicing position used by crc32c_table. */
static uint32_t derive_entry(uint8_t byte) {
	uint32_t crc = byte;
	for (int bit = 0; bit < 8; bit++) {
		if (crc & 1u)
			crc = (crc >> 1) ^ CRC32C_POLY;
		else
			crc >>= 1;
	}
	return crc;
}

/* Table derived at startup from the polynomial above. No hardcoded table. */
static uint32_t table[256];

void crc32c_table_init(void) {
	for (int i = 0; i < 256; i++)
		table[i] = derive_entry((uint8_t)i);
}

int crc32c_table_check(void) {
	for (int i = 0; i < 256; i++) {
		if (table[i] != derive_entry((uint8_t)i))
			return i;
	}
	return -1;
}

uint32_t crc32c_table(const uint8_t *data, size_t len) {
	uint32_t crc = 0xFFFFFFFFu;
	for (size_t i = 0; i < len; i++)
		crc = table[(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
	return crc ^ 0xFFFFFFFFu;
}
