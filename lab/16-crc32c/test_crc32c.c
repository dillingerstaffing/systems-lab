#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "crc32c.h"

static int failures = 0;

static void check(const char *name, uint32_t got, uint32_t want) {
	if (got != want) {
		printf("FAIL %s: got 0x%08x want 0x%08x\n", name, got, want);
		failures++;
	} else {
		printf("ok   %s = 0x%08x\n", name, got);
	}
}

struct vec {
	const char *name;
	const uint8_t *data;
	size_t len;
	uint32_t want;
};

/* 48-byte iSCSI SCSI Read(10) Command PDU, vector from Intel's ipp
 * ippsCRC32C_8u reference documentation. */
static const uint8_t pdu48[48] = {
	0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00,
	0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x18,
	0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8_t buf_zeros[32];

static void kat(void) {
	/*
	 * Sources for the expected values, in register order:
	 *  - "": the CRC-32C residue convention, also the Linux kernel
	 *    crypto/testmgr.h crc32c vector.
	 *  - "123456789": the canonical CRC-32C check value.
	 *  - "a", "abc": published CRC-32C vectors.
	 *  - "abcdefg": Linux kernel crypto/testmgr.h crc32c vector
	 *    (digest bytes 41 f4 27 e6, little endian).
	 *  - 32 zero bytes: RFC 3720 Appendix B.2 publishes 0xAA36918A in
	 *    iSCSI wire (big-endian) byte order; 0x8A9136AA is its byte swap.
	 *  - iSCSI Read(10) PDU: the 48 input bytes are quoted verbatim from
	 *    Intel's ippsCRC32C_8u reference documentation, which publishes
	 *    0x563A96D9 in wire byte order; 0xD9963A56 is its byte swap.
	 */
	static const struct vec v[] = {
		{ "\"\"", (const uint8_t *)"", 0, 0x00000000u },
		{ "\"123456789\"", (const uint8_t *)"123456789", 9, 0xE3069283u },
		{ "\"a\"", (const uint8_t *)"a", 1, 0xC1D04330u },
		{ "\"abc\"", (const uint8_t *)"abc", 3, 0x364B3FB7u },
		{ "\"abcdefg\"", (const uint8_t *)"abcdefg", 7, 0xE627F441u },
		{ "32 zero bytes", buf_zeros, 32, 0x8A9136AAu },
		{ "iSCSI Read(10) PDU", pdu48, 48, 0xD9963A56u },
	};

	for (size_t i = 0; i < sizeof(v) / sizeof(v[0]); i++) {
		check(v[i].name, crc32c_bitwise(v[i].data, v[i].len), v[i].want);
		check(v[i].name, crc32c_table(v[i].data, v[i].len), v[i].want);
	}
}

static void differential(void) {
	uint8_t b;
	uint32_t mism = 0;

	/* Exhaustive single-byte inputs. */
	for (int i = 0; i < 256; i++) {
		b = (uint8_t)i;
		if (crc32c_bitwise(&b, 1) != crc32c_table(&b, 1))
			mism++;
	}
	printf("exhaustive 1-byte: %d mismatches of 256\n", mism);

	/* Sizes 0..64 on boundary-ish patterned buffers. */
	uint8_t buf[64];
	for (int i = 0; i < 64; i++)
		buf[i] = (uint8_t)(0x9E + 37 * i);
	for (size_t len = 0; len <= 64; len++) {
		if (crc32c_bitwise(buf, len) != crc32c_table(buf, len))
			mism++;
	}
	printf("sizes 0..64: %u mismatches of 65\n", mism);

	/* Random buffers up to 4 KiB, fixed xorshift seed. */
	uint64_t rng = 0xDEADBEEF12345678ull;
	uint8_t rbuf[4096];
	uint32_t n = 0;
	for (int t = 0; t < 100000; t++) {
		rng ^= rng << 13;
		rng ^= rng >> 7;
		rng ^= rng << 17;
		size_t len = (size_t)(rng % 4097);
		uint64_t fill = rng * 0x9E3779B97F4A7C15ull;
		for (size_t i = 0; i < len; i++) {
			fill ^= fill << 13;
			fill ^= fill >> 7;
			fill ^= fill << 17;
			rbuf[i] = (uint8_t)(fill >> 56);
		}
		if (crc32c_bitwise(rbuf, len) != crc32c_table(rbuf, len))
			mism++;
		n++;
	}
	printf("random: %u mismatches of %u buffers (up to 4 KiB)\n", mism, n);
	if (mism != 0)
		failures++;
}

static double now_s(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void throughput(void) {
	static uint8_t big[32 * 1024 * 1024];
	for (size_t i = 0; i < sizeof big; i++)
		big[i] = (uint8_t)((i * 31 + 7) & 0xFF);

	double t0 = now_s();
	uint32_t c0 = crc32c_bitwise(big, sizeof big);
	double t1 = now_s();
	uint32_t c1 = crc32c_table(big, sizeof big);
	double t2 = now_s();

	double mib = (double)sizeof big / (1024.0 * 1024.0);
	printf("bitwise : 0x%08x in %.3f s -> %.1f MiB/s\n", c0, t1 - t0,
	    mib / (t1 - t0));
	printf("table   : 0x%08x in %.3f s -> %.1f MiB/s\n", c1, t2 - t1,
	    mib / (t2 - t1));
	if (c0 != c1) {
		printf("FAIL 32 MiB digests disagree\n");
		failures++;
	}
}

int main(void) {
	crc32c_table_init();
	int bad = crc32c_table_check();
	if (bad < 0)
		printf("table derivation: all 256 entries match the polynomial\n");
	else {
		printf("FAIL table derivation: entry %d disagrees\n", bad);
		failures++;
	}

	kat();
	differential();
	throughput();

	if (failures == 0)
		printf("ALL TESTS PASSED\n");
	else
		printf("%d FAILURES\n", failures);
	return failures != 0;
}
