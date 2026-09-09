/*
 * Differential test for u16_swap/u32_swap/u64_swap.
 *
 * Ground truth: __builtin_bswap16/32/64 (used ONLY as the oracle in the
 * test; the implementation in endian.c touches only shift, OR, and
 * masks). Fixed-seed splitmix64 PRNG, so every run is reproducible.
 *
 * For every value, two independent checks:
 *   1. differential: uN_swap(v) == __builtin_bswapN(v)
 *   2. involution:   uN_swap(uN_swap(v)) == v
 */
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "endian.h"

#define RANDOM_PER_WIDTH 2000000u
#define TIMED_VALUES 100000000u
#define SEED 0x123456789ABCDEF0ull

static uint64_t rng_state;

static uint64_t splitmix64(void)
{
	uint64_t z = (rng_state += 0x9E3779B97F4A7C15ull);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
	return z ^ (z >> 31);
}

static double now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static unsigned long mismatches;
static unsigned long total_cases;
static uint64_t checksum;

static void check16(uint16_t v)
{
	uint16_t s = u16_swap(v);
	if (s != __builtin_bswap16(v)) {
		mismatches++;
		printf("MISMATCH16 v=0x%04X got=0x%04X want=0x%04X\n",
		       v, s, __builtin_bswap16(v));
	}
	if (u16_swap(s) != v) {
		mismatches++;
		printf("INVOLUTION16 v=0x%04X\n", v);
	}
	total_cases += 2;
	checksum ^= ((uint64_t)s << 32) | (uint64_t)v;
}

static void check32(uint32_t v)
{
	uint32_t s = u32_swap(v);
	if (s != __builtin_bswap32(v)) {
		mismatches++;
		printf("MISMATCH32 v=0x%08X got=0x%08X want=0x%08X\n",
		       v, s, __builtin_bswap32(v));
	}
	if (u32_swap(s) != v) {
		mismatches++;
		printf("INVOLUTION32 v=0x%08X\n", v);
	}
	total_cases += 2;
	checksum ^= ((uint64_t)s << 32) | (uint64_t)v;
}

static void check64(uint64_t v)
{
	uint64_t s = u64_swap(v);
	if (s != __builtin_bswap64(v)) {
		mismatches++;
		printf("MISMATCH64 v=0x%016llX got=0x%016llX want=0x%016llX\n",
		       (unsigned long long)v,
		       (unsigned long long)s,
		       (unsigned long long)__builtin_bswap64(v));
	}
	if (u64_swap(s) != v) {
		mismatches++;
		printf("INVOLUTION64 v=0x%016llX\n", (unsigned long long)v);
	}
	total_cases += 2;
	checksum ^= s ^ v;
}

int main(void)
{
	unsigned long w16, w32, w64, wrand;
	uint32_t i;

	rng_state = SEED;

	/* Directed 16-bit cases: 0, all-ones, each byte position set,
	 * each bit position set, and walking patterns. */
	w16 = total_cases;
	check16(0x0000);
	check16(0xFFFF);
	check16(0x0102);
	check16(0x1020);
	check16(0x00FF);
	check16(0xFF00);
	for (i = 0; i < 2; i++)
		check16((uint16_t)(0xABu << (8 * i)));
	for (i = 0; i < 16; i++)
		check16((uint16_t)(1u << i));

	/* Directed 32-bit cases: 0, all-ones, each byte position set,
	 * each bit position set, 0x01020304-style patterns, high/low
	 * halves, and mixed masks. */
	w32 = total_cases;
	check32(0x00000000u);
	check32(0xFFFFFFFFu);
	check32(0x01020304u);
	check32(0x04030201u);
	check32(0xDEADBEEFu);
	check32(0x00FF00FFu);
	check32(0xFF00FF00u);
	check32(0x0000FFFFu);
	check32(0xFFFF0000u);
	check32(0x80000001u);
	for (i = 0; i < 4; i++)
		check32(0xCDu << (8 * i));
	for (i = 0; i < 32; i++)
		check32(1u << i);

	/* Directed 64-bit cases: 0, all-ones, each byte position set,
	 * each bit position set, the 0x0102..08 pattern, high/low 32-bit
	 * halves, and mixed masks. */
	w64 = total_cases;
	check64(0x0000000000000000ull);
	check64(0xFFFFFFFFFFFFFFFFull);
	check64(0x0102030405060708ull);
	check64(0x0807060504030201ull);
	check64(0xDEADBEEFCAFEBABEull);
	check64(0x00FF00FF00FF00FFull);
	check64(0xFF00FF00FF00FF00ull);
	check64(0x00000000FFFFFFFFull);
	check64(0xFFFFFFFF00000000ull);
	check64(0x8000000000000001ull);
	for (i = 0; i < 8; i++)
		check64(0xEFull << (8 * i));
	for (i = 0; i < 64; i++)
		check64(1ull << i);
	wrand = total_cases;

	printf("directed16=%lu directed32=%lu directed64=%lu\n",
	       w32 - w16, w64 - w32, wrand - w64);

	/* Random: fixed-seed values, both checks each. */
	rng_state = SEED;
	for (i = 0; i < RANDOM_PER_WIDTH; i++)
		check16((uint16_t)splitmix64());
	rng_state = SEED;
	for (i = 0; i < RANDOM_PER_WIDTH; i++)
		check32((uint32_t)splitmix64());
	rng_state = SEED;
	for (i = 0; i < RANDOM_PER_WIDTH; i++)
		check64(splitmix64());

	printf("total_cases=%lu mismatches=%lu checksum=%llu\n",
	       total_cases, mismatches, (unsigned long long)checksum);

	/* Throughput: fresh fixed-seed values through u64_swap, timed with
	 * CLOCK_MONOTONIC. The loop includes the splitmix64 step per value
	 * and accumulates into a checksum so the compiler cannot delete it,
	 * so ns/value is a ceiling for the combined loop, not a pure
	 * u64_swap measurement. */
	{
		double t0, t1;
		uint64_t acc = 0;

		rng_state = SEED ^ 0x9E3779B97F4A7C15ull;
		t0 = now_ns();
		for (i = 0; i < TIMED_VALUES; i++)
			acc ^= u64_swap(splitmix64());
		t1 = now_ns();
		printf("timed_values=%u ns_total=%.0f ns_per_value=%.2f "
		       "Mvalues_per_sec=%.1f\n",
		       TIMED_VALUES, t1 - t0, (t1 - t0) / TIMED_VALUES,
		       (TIMED_VALUES * 1000.0) / (t1 - t0));
		printf("checksum=%llu\n", (unsigned long long)acc);
	}

	if (mismatches != 0) {
		printf("FAIL\n");
		return 1;
	}
	printf("PASS\n");
	return 0;
}
