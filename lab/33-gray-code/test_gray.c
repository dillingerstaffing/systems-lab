#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

#include "gray.h"

/* Independent reference: per-bit Gray encode/decode loops. */
static uint16_t ref_encode(uint16_t n)
{
	uint16_t g = 0;
	for (int k = 0; k < 16; k++) {
		uint16_t nk = (uint16_t)((n >> k) & 1u);
		uint16_t nk1 = (k == 15) ? 0u : (uint16_t)((n >> (k + 1)) & 1u);
		g |= (uint16_t)((nk ^ nk1) << k);
	}
	return g;
}

static uint16_t ref_decode(uint16_t g)
{
	uint16_t b = 0;
	uint16_t prev = 0; /* b_{k+1}, with b_16 = 0 */
	for (int k = 15; k >= 0; k--) {
		uint16_t gk = (uint16_t)((g >> k) & 1u);
		uint16_t bk = (uint16_t)(gk ^ prev);
		b |= (uint16_t)(bk << k);
		prev = bk;
	}
	return b;
}

/* Naive reference for the single-bit-adjacency invariant. */
static int ref_popcount16(uint16_t x)
{
	int c = 0;
	for (int k = 0; k < 16; k++)
		c += (int)((x >> k) & 1u);
	return c;
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
 * Timed loop: accumulates encode/decode results into volatile sinks so the
 * compiler cannot drop the work. The loop includes one splitmix64 PRNG
 * step per value, so the measured rate is a ceiling on the raw rate.
 */
static double measure_encode(void)
{
	volatile uint16_t sink = 0;
	uint64_t s = 0x123456789ABCDEF0ull;
	const uint64_t N = 100000000ull;
	double t0 = ns_now();
	for (uint64_t i = 0; i < N; i++) {
		uint16_t v = (uint16_t)splitmix64(&s);
		sink ^= gray16_encode(v);
	}
	double t1 = ns_now();
	(void)sink;
	return (t1 - t0) / (double)N;
}

static double measure_decode(void)
{
	volatile uint16_t sink = 0;
	uint64_t s = 0xFEDCBA9876543210ull;
	const uint64_t N = 100000000ull;
	double t0 = ns_now();
	for (uint64_t i = 0; i < N; i++) {
		uint16_t v = (uint16_t)splitmix64(&s);
		sink ^= gray16_decode(v);
	}
	double t1 = ns_now();
	(void)sink;
	return (t1 - t0) / (double)N;
}
#endif

int main(void)
{
	uint64_t enc_checks = 0, dec_checks = 0, inv_checks = 0, adj_checks = 0;
	uint64_t enc_mism = 0, dec_mism = 0, inv_mism = 0, adj_mism = 0;
	uint64_t checksum = 0xcbf29ce484222325ull; /* FNV-1a offset basis */

	for (uint32_t x32 = 0; x32 < 65536u; x32++) {
		uint16_t x = (uint16_t)x32;

		uint16_t enc = gray16_encode(x);
		uint16_t enc_ref = ref_encode(x);
		enc_checks++;
		if (enc != enc_ref)
			enc_mism++;

		uint16_t dec = gray16_decode(x);
		uint16_t dec_ref = ref_decode(x);
		dec_checks++;
		if (dec != dec_ref)
			dec_mism++;

		uint16_t roundtrip = gray16_decode(enc);
		inv_checks++;
		if (roundtrip != x)
			inv_mism++;

		if (x < 65535u) {
			uint16_t a = enc;
			uint16_t b = gray16_encode((uint16_t)(x + 1));
			adj_checks++;
			if (ref_popcount16((uint16_t)(a ^ b)) != 1)
				adj_mism++;
		}

		/* Fold results into an FNV-1a checksum for build consistency. */
		for (int k = 0; k < 4; k++) {
			checksum ^= (uint64_t)(uint8_t)(enc >> (k * 8)) & 0xffull;
			checksum *= 0x100000001b3ull;
			checksum ^= (uint64_t)(uint8_t)(dec >> (k * 8)) & 0xffull;
			checksum *= 0x100000001b3ull;
		}
	}

	printf("encode differential (impl vs bit-loop ref): %" PRIu64
	       " checks, %" PRIu64 " mismatches\n", enc_checks, enc_mism);
	printf("decode differential (impl vs bit-loop ref): %" PRIu64
	       " checks, %" PRIu64 " mismatches\n", dec_checks, dec_mism);
	printf("involution decode(encode(x)) == x: %" PRIu64
	       " checks, %" PRIu64 " mismatches\n", inv_checks, inv_mism);
	printf("adjacent single-bit change: %" PRIu64
	       " checks, %" PRIu64 " mismatches\n", adj_checks, adj_mism);
	printf("FNV-1a checksum: %" PRIu64 "\n", checksum);

#ifdef MEASURE
	printf("encode throughput: %.2f ns/value (100M values)\n", measure_encode());
	printf("decode throughput: %.2f ns/value (100M values)\n", measure_decode());
#endif

	return (enc_mism || dec_mism || inv_mism || adj_mism) ? 1 : 0;
}
