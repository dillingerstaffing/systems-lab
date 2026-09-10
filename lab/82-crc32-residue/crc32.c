#include "crc32.h"

/*
 * CRC32/IEEE 802.3 as specified by the generator polynomial
 *   G(x) = x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10
 *        + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
 * i.e. the 33-bit value 0x104C11DB7. The top bit is implicit, so it is
 * written as the 32-bit constant 0x04C11DB7.
 *
 * The computation below works on reflected (LSB-first) data: reflecting
 * the message, the register and the polynomial turns the division into
 * shift-right steps, and the reflected polynomial is 0xEDB88320
 * (bit-reverse of 0x04C11DB7).
 *
 * xorIn  = 0xFFFFFFFF: the register starts with all 1s.
 * xorOut = 0xFFFFFFFF: the final register is inverted before use.
 */
#define POLY_REFLECTED 0xEDB88320u
#define CRC_INIT 0xFFFFFFFFu
#define CRC_XOROUT 0xFFFFFFFFu

/* Variant A: polynomial long division over GF(2), one bit per step.
 * Each message byte is folded into the low 8 bits of the register,
 * then the register is divided by G(x): whenever a 1 shifts out of
 * the low end, xor the reflected polynomial back in. Subtraction in
 * GF(2) is xor, so the "subtract multiple of G" step is one xor. */
uint32_t crc32_bitwise(const void *data, size_t len)
{
	const uint8_t *p = (const uint8_t *)data;
	uint32_t crc = CRC_INIT;

	while (len--) {
		crc ^= *p++;
		for (int k = 0; k < 8; k++) {
			if (crc & 1u)
				crc = (crc >> 1) ^ POLY_REFLECTED;
			else
				crc >>= 1;
		}
	}
	return crc ^ CRC_XOROUT;
}

/* Variant B: 8 bits per step. The 256-entry table is not a lookup of
 * precomputed magic numbers: each entry is what the register becomes
 * after dividing one byte by G(x), derived below from POLY_REFLECTED
 * with the exact same 8 shift/xor steps as the bitwise variant. */
static uint32_t crc32_table_data[256];
static int table_ready;

static void crc32_build_table(void)
{
	for (uint32_t i = 0; i < 256; i++) {
		uint32_t e = i;
		for (int k = 0; k < 8; k++) {
			if (e & 1u)
				e = (e >> 1) ^ POLY_REFLECTED;
			else
				e >>= 1;
		}
		crc32_table_data[i] = e;
	}
	table_ready = 1;
}

uint32_t crc32_table(const void *data, size_t len)
{
	const uint8_t *p = (const uint8_t *)data;
	uint32_t crc = CRC_INIT;

	if (!table_ready)
		crc32_build_table();

	while (len--)
		crc = crc32_table_data[(crc ^ *p++) & 0xFFu] ^ (crc >> 8);
	return crc ^ CRC_XOROUT;
}

/* The test calls this to confirm the test harness itself agrees with
 * the check value for the canonical 9-byte ASCII sequence. */
uint32_t crc32_reflected_check(void)
{
	static const uint8_t v[] = "123456789";
	return crc32_table(v, 9);
}
