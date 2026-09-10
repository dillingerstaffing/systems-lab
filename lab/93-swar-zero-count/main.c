#define _POSIX_C_SOURCE 199309L

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "zero_count.h"

/* Reference: naive per-byte comparison loop. */
static unsigned ref_count_zero_bytes(uint64_t x) {
	unsigned n = 0;
	for (int i = 0; i < 8; i++) {
		if (((x >> (8 * i)) & 0xFFu) == 0)
			n++;
	}
	return n;
}

/* splitmix64: fixed deterministic PRNG, state seeded from main. */
static uint64_t splitmix64(uint64_t *state) {
	uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

#define FNV_OFFSET 0xCBF29CE484222325ULL
#define FNV_PRIME  0x100000001B3ULL

int main(void) {
	uint64_t mismatches = 0;
	uint64_t checked = 0;
	uint64_t fnv = FNV_OFFSET;

	/* Exhaustive: every 16-bit input, zero-extended to 64 bits. */
	for (uint64_t x = 0; x < 65536; x++) {
		unsigned a = count_zero_bytes(x);
		unsigned b = ref_count_zero_bytes(x);
		if (a != b)
			mismatches++;
		checked++;
		fnv ^= (uint64_t)a;
		fnv *= FNV_PRIME;
	}

	/* Fixed-seed random: 10,000,000 full 64-bit words. */
	uint64_t state = 0x123456789ABCDEF0ULL;
	for (uint64_t i = 0; i < 10000000; i++) {
		uint64_t x = splitmix64(&state);
		unsigned a = count_zero_bytes(x);
		unsigned b = ref_count_zero_bytes(x);
		if (a != b)
			mismatches++;
		checked++;
		fnv ^= (uint64_t)a;
		fnv *= FNV_PRIME;
	}

	printf("checked=%llu mismatches=%llu fnv1a=%016llx\n",
	       (unsigned long long)checked, (unsigned long long)mismatches,
	       (unsigned long long)fnv);

	/* Throughput: precomputed fixed-seed words, checksum prevents dead-code elim. */
	const uint64_t K = 131072ULL;
	static uint64_t buf[131072];
	state = 0x123456789ABCDEF0ULL;
	for (uint64_t i = 0; i < K; i++)
		buf[i] = splitmix64(&state);
	const uint64_t reps = 200ULL;
	uint64_t timed = K * reps;
	uint64_t acc = 0;
	double best_ns = 1e300;
	for (int r = 0; r < 5; r++) {
		struct timespec t0, t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		uint64_t sum = 0;
		for (uint64_t rep = 0; rep < reps; rep++)
			for (uint64_t i = 0; i < K; i++)
				sum += count_zero_bytes(buf[i]);
		clock_gettime(CLOCK_MONOTONIC, &t1);
		double sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
		double ns = sec * 1e9 / (double)timed;
		if (ns < best_ns)
			best_ns = ns;
		acc += sum;
	}
	printf("timed=%llu best=%.3f ns/value checksum=%llu\n",
	       (unsigned long long)timed, best_ns, (unsigned long long)acc);
	return mismatches == 0 ? 0 : 1;
}
