#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "roundup.h"

/*
 * Independent reference: a plain division loop with no bit tricks.
 * Contract it implements: 0 -> 0; otherwise the smallest power of
 * two >= x; 0 when that power of two would exceed 64 bits (inputs
 * above 2^63), because 2^64 is unrepresentable in uint64_t.
 */
static uint64_t round_up_pow2_64_ref(uint64_t x) {
	if (x == 0) {
		return 0;
	}
	uint64_t p = 1;
	while (p < x) {
		if (p > UINT64_MAX / 2u) {
			return 0; /* doubling p would exceed 64 bits */
		}
		p *= 2u;
	}
	return p;
}

/* splitmix64: fixed-seed PRNG for the random case phase. */
static uint64_t splitmix64(uint64_t *state) {
	uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

/* FNV-1a 64 over a 64-bit word, for the cross-build checksum. */
static uint64_t fnv1a(uint64_t h, uint64_t v) {
	h ^= v;
	h *= 0x100000001B3ULL;
	return h;
}

static int failures = 0;
static long mismatches = 0;
static uint64_t checksum = 0xCBF29CE484222325ULL;

/* One dedicated contract row: the implementation must equal the
 * expected value derived from the identity, and the independent
 * reference must agree with the same expected value. */
static void check_dedicated(uint64_t x, uint64_t want) {
	uint64_t got = round_up_pow2_64(x);
	uint64_t ref = round_up_pow2_64_ref(x);
	checksum = fnv1a(checksum, x);
	checksum = fnv1a(checksum, got);
	if (got != want || ref != want) {
		printf("FAIL x=%" PRIu64 " got=%" PRIu64 " ref=%" PRIu64 " want=%" PRIu64 "\n",
		       x, got, ref, want);
		failures++;
		mismatches++;
	}
}

static double now_ns(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void) {
	int k;
	long dedicated_rows = 0;

	/* Phase 1: dedicated contract rows. */
	check_dedicated(0, 0); /* 0 -> 0: (0 - 1) wraps to all ones, +1 wraps back */
	dedicated_rows++;
	for (k = 0; k < 64; k++) {
		/* 2^k is identity: (2^k - 1) is already all lower bits set. */
		check_dedicated(1ULL << k, 1ULL << k);
		dedicated_rows++;
	}
	for (k = 2; k < 64; k++) {
		/* 2^k - 1 rounds up to 2^k. (k starts at 2: for k = 1,
		 * 2^k - 1 = 1 is itself a power of two, already covered
		 * by the identity rows above.) */
		check_dedicated((1ULL << k) - 1u, 1ULL << k);
		dedicated_rows++;
	}
	for (k = 0; k < 63; k++) {
		/* 2^k + 1 rounds up to 2^(k+1). */
		check_dedicated((1ULL << k) + 1u, 1ULL << (k + 1));
		dedicated_rows++;
	}
	/* Inputs above 2^63: the rounded power 2^64 is unrepresentable,
	 * so the identity wraps to 0. */
	check_dedicated((1ULL << 63) + 1u, 0);
	check_dedicated((1ULL << 63) + 12345u, 0);
	check_dedicated(UINT64_MAX - 1u, 0);
	check_dedicated(UINT64_MAX, 0);
	dedicated_rows += 4;
	if (failures == 0) {
		printf("ok   dedicated contract rows (%ld): mismatches 0\n", dedicated_rows);
	}

	/* Phase 2: exhaustive 16-bit inputs, differential vs the reference. */
	for (uint64_t x = 0; x <= 0xFFFFu; x++) {
		uint64_t got = round_up_pow2_64(x);
		uint64_t ref = round_up_pow2_64_ref(x);
		checksum = fnv1a(checksum, x);
		checksum = fnv1a(checksum, got);
		if (got != ref) {
			mismatches++;
		}
	}
	printf("ok   exhaustive 16-bit (65536 values): mismatches %" PRId64 "\n",
	       (int64_t)mismatches);

	/* Phase 3: 1,000,000 fixed-seed splitmix64 64-bit values,
	 * differential vs the reference. */
	{
		uint64_t state = 0x123456789ABCDEF0ULL;
		for (long i = 0; i < 1000000L; i++) {
			uint64_t x = splitmix64(&state);
			uint64_t got = round_up_pow2_64(x);
			uint64_t ref = round_up_pow2_64_ref(x);
			checksum = fnv1a(checksum, x);
			checksum = fnv1a(checksum, got);
			if (got != ref) {
				mismatches++;
			}
		}
		printf("random 64-bit cases: 1000000 (seed 0x123456789ABCDEF0)\n");
	}
	printf("implementation vs division-loop reference mismatches (all phases): %" PRId64 "\n",
	       (int64_t)mismatches);
	printf("fnv1a over per-case (input, result): 0x%016" PRIx64 "\n", checksum);

	/* Phase 4: throughput at this build's optimization level.
	 * PRNG generation is outside the timed region. */
	{
		const long n = 1000000L;
		uint64_t *vals = malloc((size_t)n * sizeof(*vals));
		uint64_t state = 0x123456789ABCDEF0ULL;
		uint64_t sink = 0;
		double best = 1e30;
		int r;

		if (!vals) {
			printf("FAIL malloc\n");
			return 1;
		}
		for (long i = 0; i < n; i++) {
			vals[i] = splitmix64(&state);
		}
		for (r = 0; r < 5; r++) {
			uint64_t acc = 0;
			double t0 = now_ns();
			for (long i = 0; i < n; i++) {
				acc ^= round_up_pow2_64(vals[i]);
			}
			double t1 = now_ns();
			double ns = (t1 - t0) / (double)n;
			if (ns < best) {
				best = ns;
			}
			sink = acc;
		}
		printf("timed sink (prevents dead-code elimination): %" PRIu64 "\n", sink);
		printf("throughput: %.2f ns/value (best of 5; PRNG not in timed region)\n", best);
		free(vals);
	}

	if (failures == 0 && mismatches == 0) {
		printf("RESULT: ALL TESTS PASSED\n");
		return 0;
	}
	printf("RESULT: FAILED (%d dedicated row failures, %" PRId64 " total mismatches)\n",
	       failures, (int64_t)mismatches);
	return 1;
}
