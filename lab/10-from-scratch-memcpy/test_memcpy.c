#define _POSIX_C_SOURCE 199309L
/*
 * Differential test for memcpy10 against the libc memcpy oracle.
 *
 * Coverage:
 *   - exhaustive: sizes 0..64 x dest misalignment 0..7 x source
 *     misalignment 0..7 (65 * 8 * 8 = 4160 cases)
 *   - random: 1,000,000 cases with random size (biased small, some up to
 *     4096, occasional 65536), random misalignments 0..7, random bytes
 *   - zero length, size 1, size = WORD_BYTES -/+ 1 included in the above
 *   - return value must equal dest
 *   - a separate throughput run measures memcpy10 vs libc memcpy on a
 *     64 MiB copy
 *
 * Overlapping regions are out of scope (same as libc memcpy: UB).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "memcpy10.h"

#define BUF (65536 + 16)
#define RANDOM_CASES 1000000

static unsigned long long rng_state = 0x9E3779B97F4A7C15ULL;

static unsigned long long rng(void)
{
	unsigned long long x = rng_state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	rng_state = x;
	return x;
}

static unsigned char s1[BUF], s2[BUF], ref[BUF];

static unsigned long long fails;

/* One differential case: copy n bytes with the given misalignments of
 * dest and src inside the buffers, compare against libc memcpy. */
static void one_case(unsigned char *src, unsigned char *dst_a,
		     unsigned char *dst_b, size_t n, unsigned dmis,
		     unsigned smis)
{
	unsigned char *s = src + smis;
	unsigned char *a = dst_a + dmis;
	unsigned char *b = dst_b + dmis;
	void *ret;

	ret = memcpy10(a, s, n);
	if (ret != a) {
		printf("FAIL: return value != dest (n=%zu dmis=%u smis=%u)\n",
		       n, dmis, smis);
		fails++;
		return;
	}
	memcpy(b, s, n);
	if (memcmp(a, b, n) != 0) {
		printf("FAIL: byte mismatch (n=%zu dmis=%u smis=%u)\n",
		       n, dmis, smis);
		fails++;
	}
}

int main(void)
{
	unsigned long long cases = 0;
	struct timespec t0, t1;
	double mine_s, libc_s, mib;

	/* Deterministic content so failures are reproducible. */
	for (size_t i = 0; i < BUF; i++)
		s1[i] = (unsigned char)(rng() & 0xFF);

	/* Exhaustive small-size sweep. */
	for (size_t n = 0; n <= 64; n++)
		for (unsigned dm = 0; dm < 8; dm++)
			for (unsigned sm = 0; sm < 8; sm++) {
				one_case(s1, s2, ref, n, dm, sm);
				cases++;
			}

	/* One million random cases. */
	for (unsigned long long k = 0; k < RANDOM_CASES; k++) {
		size_t n;
		unsigned dm, sm;
		unsigned r = (unsigned)(rng() & 0xFF);

		if (r < 192)
			n = (size_t)(rng() % 65);	/* 0..64, bias small */
		else if (r < 240)
			n = (size_t)(rng() % 4097);	/* up to 4096 */
		else
			n = 65536;			/* big block */

		dm = (unsigned)(rng() & 7);
		sm = (unsigned)(rng() & 7);
		if (n + dm >= BUF || n + sm >= BUF)
			n = BUF - 16;
		one_case(s1, s2, ref, n, dm, sm);
		cases++;
	}

	printf("differential: %llu cases, %llu mismatches\n", cases, fails);

	/* Throughput: 64 MiB copy, 8-byte aligned, timed with clock_gettime. */
	{
		static unsigned char big_src[64 * 1024 * 1024];
		static unsigned char big_dst[64 * 1024 * 1024];
		size_t big = sizeof(big_src);
		int reps = 20;
		unsigned long long acc;

		for (size_t i = 0; i < big; i++)
			big_src[i] = (unsigned char)(i * 2654435761u >> 16);

		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (int r = 0; r < reps; r++)
			memcpy10(big_dst, big_src, big);
		clock_gettime(CLOCK_MONOTONIC, &t1);
		mine_s = (t1.tv_sec - t0.tv_sec) +
			 (t1.tv_nsec - t0.tv_nsec) / 1e9;
		acc = big_dst[0] + big_dst[big - 1];

		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (int r = 0; r < reps; r++)
			memcpy(big_dst, big_src, big);
		clock_gettime(CLOCK_MONOTONIC, &t1);
		libc_s = (t1.tv_sec - t0.tv_sec) +
			 (t1.tv_nsec - t0.tv_nsec) / 1e9;
		acc += big_dst[0] + big_dst[big - 1];

		mib = (double)big * reps / (1024.0 * 1024.0);
		printf("throughput (64 MiB x %d, checksum %llu): memcpy10 %.1f MiB/s, libc memcpy %.1f MiB/s\n",
		       reps, acc % 1000, mib / mine_s, mib / libc_s);
	}

	return fails == 0 ? 0 : 1;
}
