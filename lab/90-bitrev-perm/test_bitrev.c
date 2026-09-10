#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bitrev.h"

/*
 * Independent reference: bit reversal computed by an explicit
 * per-bit loop, a different code path from the swap construction
 * in bitrev.c. Contract: for a 3-bit index i, the reference
 * reversal is sum over bit positions b of (((i >> b) & 1) << (2-b)).
 */
static unsigned bitrev3_ref(unsigned i) {
	unsigned r = 0;
	for (unsigned b = 0; b < 3; b++) {
		r |= ((i >> b) & 1u) << (2 - b);
	}
	return r;
}

static void bitrev8_perm_ref(const uint32_t in[8], uint32_t out[8]) {
	for (unsigned i = 0; i < 8; i++) {
		out[bitrev3_ref(i)] = in[i];
	}
}

/* xorshift64* PRNG, seeded deterministically for reproducibility. */
static uint64_t rng_state = 0x9E3779B97F4A7C15ull;

static uint64_t rng_next(void) {
	uint64_t x = rng_state;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	rng_state = x;
	return x * 0x2545F4914F6CDD1Dull;
}

/* Fill dst with 8 distinct 32-bit values. */
static void rand_distinct8(uint32_t dst[8]) {
	unsigned n = 0;
	while (n < 8) {
		uint32_t v = (uint32_t)(rng_next() >> 33);
		unsigned dup = 0;
		for (unsigned k = 0; k < n; k++) {
			if (dst[k] == v) {
				dup = 1;
				break;
			}
		}
		if (!dup) {
			dst[n++] = v;
		}
	}
}

static uint64_t fnv1a64(const void *p, size_t n, uint64_t h) {
	const unsigned char *b = (const unsigned char *)p;
	for (size_t i = 0; i < n; i++) {
		h ^= b[i];
		h *= 0x100000001B3ull;
	}
	return h;
}

static double now_sec(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int main(void) {
	const long cases = 1000000L;
	uint32_t in[8], out[8], ref[8], twice[8];
	long diff_mismatches = 0;   /* (1) implementation vs reference */
	long bij_mismatches = 0;    /* (2) permutation is a bijection */
	long inv_mismatches = 0;    /* (3) applying it twice is identity */
	uint64_t fnv = 0xCBF29CE484222325ull;

	for (long c = 0; c < cases; c++) {
		rand_distinct8(in);
		bitrev8_perm(in, out);
		bitrev8_perm_ref(in, ref);

		for (unsigned i = 0; i < 8; i++) {
			if (out[i] != ref[i]) {
				diff_mismatches++;
			}
		}

		/* Bijection: inputs are 8 distinct values, so the output
		 * must contain each input value exactly once. */
		for (unsigned i = 0; i < 8; i++) {
			unsigned cnt = 0;
			for (unsigned j = 0; j < 8; j++) {
				cnt += (out[j] == in[i]);
			}
			if (cnt != 1) {
				bij_mismatches++;
			}
		}

		/* Involution: applying the permutation twice restores
		 * the original ordering. */
		bitrev8_perm(out, twice);
		for (unsigned i = 0; i < 8; i++) {
			if (twice[i] != in[i]) {
				inv_mismatches++;
			}
		}

		fnv = fnv1a64(out, sizeof(out), fnv);
	}

	/* Timed region: only the permutation, PRNG outside of it.
	 * Best of 5 runs, ns per array element. */
	const long timed_cases = 2000000L;
	double best = 1e100;
	uint64_t sink = 0;
	for (int rep = 0; rep < 5; rep++) {
		rand_distinct8(in); /* fixed input; PRNG outside the timed region */
		double t0 = now_sec();
		for (long c = 0; c < timed_cases; c++) {
			bitrev8_perm(in, out);
			sink += out[0] + out[7];
		}
		double t1 = now_sec();
		double ns = (t1 - t0) * 1e9 / ((double)timed_cases * 8.0);
		if (ns < best) {
			best = ns;
		}
	}

	printf("randomized 8-element arrays of distinct values: %ld (seed 0x9E3779B97F4A7C15)\n", cases);
	printf("implementation vs per-bit-loop reference mismatches: %ld\n", diff_mismatches);
	printf("bijection check mismatches: %ld\n", bij_mismatches);
	printf("apply-twice identity mismatches: %ld\n", inv_mismatches);
	printf("fnv1a over all outputs: 0x%016" PRIx64 "\n", fnv);
	printf("timed sink (prevents dead-code elimination): %" PRIu64 "\n", sink);
	printf("throughput: %.2f ns/value (best of 5; PRNG outside timed region)\n", best);
	if (diff_mismatches == 0 && bij_mismatches == 0 && inv_mismatches == 0) {
		printf("RESULT: ALL TESTS PASSED\n");
		return 0;
	}
	printf("RESULT: FAILURES PRESENT\n");
	return 1;
}
