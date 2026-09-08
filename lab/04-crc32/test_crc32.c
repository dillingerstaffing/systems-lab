#define _POSIX_C_SOURCE 200809L /* for clock_gettime */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "crc32.h"

/* Known-answer vectors for CRC32/IEEE 802.3. The "123456789" check
 * value 0xCBF43926 is the published canonical check value. */
struct kat {
	const char *input;
	uint32_t expected;
};

static const struct kat kats[] = {
	{ "",          0x00000000u },
	{ "a",         0xE8B7BE43u },
	{ "abc",       0x352441C2u },
	{ "message digest", 0x20159D7Fu },
	{ "abcdefghijklmnopqrstuvwxyz", 0x4C2750BDu },
	{ "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", 0x1FC2E6D2u },
	{ "123456789", 0xCBF43926u },
};

/* Small deterministic PRNG (xorshift32) so the cross-check inputs are
 * reproducible without relying on libc rand. */
static uint32_t xs_state;

static uint32_t xs_next(void)
{
	uint32_t x = xs_state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xs_state = x;
	return x;
}

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int fails = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		printf("FAIL: "); \
		printf(__VA_ARGS__); \
		printf("\n"); \
		fails++; \
	} \
} while (0)

int main(void)
{
	printf("crc32 test: two implementations, known-answer + cross-check + timing\n");

	/* 1. Known-answer tests, both variants, every vector. */
	for (size_t i = 0; i < sizeof kats / sizeof kats[0]; i++) {
		size_t len = strlen(kats[i].input);
		uint32_t b = crc32_bitwise(kats[i].input, len);
		uint32_t t = crc32_table(kats[i].input, len);
		CHECK(b == kats[i].expected,
		      "bitwise KAT[%zu] got 0x%08X want 0x%08X", i, b, kats[i].expected);
		CHECK(t == kats[i].expected,
		      "table KAT[%zu] got 0x%08X want 0x%08X", i, t, kats[i].expected);
		CHECK(b == t,
		      "bitwise/table disagree on KAT[%zu]: 0x%08X vs 0x%08X", i, b, t);
	}
	printf("known-answer vectors: %zu inputs x 2 implementations, all match\n",
	       sizeof kats / sizeof kats[0]);

	/* 2. Cross-check: the two variants must agree bit-for-bit on
	 * randomized buffers of every length 0..256 plus larger blocks. */
	uint8_t *buf = malloc(65536);
	if (!buf) {
		printf("FAIL: malloc\n");
		return 1;
	}
	long cross = 0;
	for (uint32_t seed = 1; seed <= 8; seed++) {
		xs_state = seed ? seed : 1;
		for (size_t len = 0; len <= 256; len++) {
			for (size_t i = 0; i < len; i++)
				buf[i] = (uint8_t)xs_next();
			uint32_t b = crc32_bitwise(buf, len);
			uint32_t t = crc32_table(buf, len);
			CHECK(b == t, "cross-check seed=%u len=%zu: 0x%08X vs 0x%08X",
			      seed, len, b, t);
			cross++;
		}
		/* Larger blocks: 1, 4, 16, 64 KiB of fresh random bytes. */
		for (int b_ = 0; b_ < 4; b_++) {
			size_t len = (size_t)1024 << (2 * b_);
			for (size_t i = 0; i < len; i++)
				buf[i] = (uint8_t)xs_next();
			uint32_t b = crc32_bitwise(buf, len);
			uint32_t t = crc32_table(buf, len);
			CHECK(b == t, "cross-check seed=%u len=%zu: 0x%08X vs 0x%08X",
			      seed, len, b, t);
			cross++;
		}
	}
	printf("cross-check: %ld random buffers, bitwise == table on all\n", cross);

	/* 3. Table sanity: the generated table must reproduce its own
	 * definition, one entry at a time, against the bitwise routine. */
	for (uint32_t i = 0; i < 256; i++) {
		uint8_t byte = (uint8_t)i;
		uint32_t b = crc32_bitwise(&byte, 1);
		uint32_t t = crc32_table(&byte, 1);
		CHECK(b == t, "single-byte 0x%02X mismatch: 0x%08X vs 0x%08X", i, b, t);
	}
	printf("table derivation: all 256 single-byte CRCs match the bitwise result\n");

	if (fails) {
		printf("FAILURES: %d\n", fails);
		free(buf);
		return 1;
	}
	printf("ALL CORRECTNESS TESTS PASSED\n\n");

	/* 4. Timing: throughput of each variant on a 32 MiB buffer of
	 * random bytes, averaged over repeated runs, best-of and mean. */
	const size_t big = 32 * 1024 * 1024;
	xs_state = 0xC0FFEEu;
	free(buf);
	buf = malloc(big);
	if (!buf) {
		printf("FAIL: malloc 32MiB\n");
		return 1;
	}
	for (size_t i = 0; i < big; i++)
		buf[i] = (uint8_t)xs_next();
	/* Warm up so the table build and caches settle before timing. */
	crc32_table(buf, big);
	crc32_bitwise(buf, 1024);

	const int runs = 5;
	uint32_t sink = 0;

	double t0 = now_sec();
	for (int r = 0; r < runs; r++)
		sink ^= crc32_bitwise(buf, big);
	double t1 = now_sec();
	double bitwise_mb_s = (double)big * runs / (t1 - t0) / 1e6;

	t0 = now_sec();
	for (int r = 0; r < runs; r++)
		sink ^= crc32_table(buf, big);
	t1 = now_sec();
	double table_mb_s = (double)big * runs / (t1 - t0) / 1e6;

	printf("timing: 32 MiB buffer, %d runs each, sink=0x%08X\n", runs, sink);
	printf("  bitwise: %.1f MB/s (%.2f Gbps)\n", bitwise_mb_s, bitwise_mb_s * 8 / 1000);
	printf("  table:   %.1f MB/s (%.2f Gbps)\n", table_mb_s, table_mb_s * 8 / 1000);
	printf("  speedup: %.1fx\n", table_mb_s / bitwise_mb_s);

	free(buf);
	printf("ALL TESTS PASSED\n");
	return 0;
}
