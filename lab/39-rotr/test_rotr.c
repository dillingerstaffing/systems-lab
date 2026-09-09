#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "rotr.h"

/*
 * Independent reference: naive per-bit reconstruction of a rotate right.
 * Output bit k is input bit (k + r) mod 64. No shifts of combined values,
 * no wrap tricks; it reads each source bit individually and places it.
 */
static uint64_t ref_rotr64(uint64_t x, unsigned r)
{
	uint64_t out = 0;
	for (unsigned k = 0; k < 64u; k++) {
		uint64_t bit = (x >> ((k + r) % 64u)) & 1ull;
		out |= bit << k;
	}
	return out;
}

#ifdef MEASURE
static uint64_t splitmix64(uint64_t *s)
{
	uint64_t z = (*s += 0x9E3779B97F4A7C15ull);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	return z ^ (z >> 31);
}

static double ns_now(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/*
 * Timed loop: accumulates rotate results into a volatile sink so the
 * compiler cannot drop the work. Each iteration does one splitmix64 PRNG
 * step plus one rotation, so the measured rate is a ceiling on the raw
 * rotation rate.
 */
static double measure_rotr(void)
{
	volatile uint64_t sink = 0;
	uint64_t s = 0x123456789ABCDEF0ull;
	const uint64_t N = 100000000ull;
	double t0 = ns_now();
	for (uint64_t i = 0; i < N; i++) {
		uint64_t v = splitmix64(&s);
		sink ^= rotr64(v, (unsigned)(i & 63u));
	}
	double t1 = ns_now();
	(void)sink;
	return (t1 - t0) / (double)N;
}

static double measure_rotl(void)
{
	volatile uint64_t sink = 0;
	uint64_t s = 0xFEDCBA9876543210ull;
	const uint64_t N = 100000000ull;
	double t0 = ns_now();
	for (uint64_t i = 0; i < N; i++) {
		uint64_t v = splitmix64(&s);
		sink ^= rotl64(v, (unsigned)(i & 63u));
	}
	double t1 = ns_now();
	(void)sink;
	return (t1 - t0) / (double)N;
}
#endif

int main(void)
{
	uint64_t diff_checks = 0, diff_mism = 0;
	uint64_t inv_checks = 0, inv_mism = 0;
	uint64_t checksum = 0xcbf29ce484222325ull; /* FNV-1a offset basis */

	/* Exhaustive over all 16-bit inputs, rotation amounts 0..31. */
	for (uint32_t x32 = 0; x32 < 65536u; x32++) {
		uint64_t x = (uint64_t)x32;
		for (unsigned r = 0; r < 32u; r++) {
			uint64_t got = rotr64(x, r);
			uint64_t want = ref_rotr64(x, r);
			diff_checks++;
			if (got != want)
				diff_mism++;

			inv_checks++;
			if (rotr64(rotl64(x, r), r) != x)
				inv_mism++;

			/* Fold result bytes into the FNV-1a checksum. */
			for (int k = 0; k < 8; k++) {
				checksum ^= (uint8_t)(got >> (k * 8)) & 0xffull;
				checksum *= 0x100000001b3ull;
			}
		}
	}

	printf("rotr differential (impl vs bit-loop ref): %" PRIu64
	       " checks, %" PRIu64 " mismatches\n", diff_checks, diff_mism);
	printf("invariant rotr(rotl(x, r), r) == x: %" PRIu64
	       " checks, %" PRIu64 " mismatches\n", inv_checks, inv_mism);
	printf("FNV-1a checksum: %" PRIu64 "\n", checksum);

#ifdef MEASURE
	printf("rotr throughput: %.2f ns/value (100M values)\n", measure_rotr());
	printf("rotl throughput: %.2f ns/value (100M values)\n", measure_rotl());
#endif

	return (diff_mism || inv_mism) ? 1 : 0;
}
