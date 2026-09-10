#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "tie.h"

/*
 * Independent reference: quotient-and-remainder decomposition of the
 * real division x / 2, with the tie resolved to the even neighbor.
 * q = x / 2 and r = x % 2 are computed by the compiler's division
 * unit, a completely separate path from the shift/AND identity in
 * the implementation. Result: r ? (q odd ? q + 1 : q) : q.
 */
static uint32_t tie_to_even_half_ref(uint32_t x) {
	uint32_t q = x / 2u;
	uint32_t r = x % 2u;
	if (r == 0u) {
		return q;
	}
	return (q & 1u) ? q + 1u : q;
}

/* FNV-1a 64 over a 32-bit word, for the cross-build checksum. */
static uint64_t fnv1a(uint64_t h, uint32_t v) {
	h ^= v;
	h *= 0x100000001B3ULL;
	return h;
}

static int failures = 0;
static uint64_t mismatches = 0;
static uint64_t checksum = 0xCBF29CE484222325ULL;

/*
 * One dedicated contract row: the implementation and the
 * independent quotient/remainder reference must both equal the
 * hand-derived expectation.
 */
static void check_dedicated(uint32_t x, uint32_t want) {
	uint32_t got = tie_to_even_half(x);
	uint32_t ref = tie_to_even_half_ref(x);
	checksum = fnv1a(checksum, x);
	checksum = fnv1a(checksum, got);
	if (got != want || ref != want) {
		printf("FAIL x=%" PRIu32 " got=%" PRIu32 " ref=%" PRIu32
		       " want=%" PRIu32 "\n",
		       x, got, ref, want);
		failures++;
	}
}

int main(void) {
	/* Phase 1: dedicated contract rows, expectations derived by hand. */
	check_dedicated(0u, 0u);            /* 0 / 2 = 0, exact */
	check_dedicated(1u, 0u);            /* tie 0.5, neighbors 0/1, even 0 */
	check_dedicated(2u, 1u);            /* 2 / 2 = 1, exact */
	check_dedicated(3u, 2u);            /* tie 1.5, neighbors 1/2, even 2 */
	check_dedicated(4u, 2u);            /* 4 / 2 = 2, exact */
	check_dedicated(5u, 2u);            /* tie 2.5, neighbors 2/3, even 2 */
	check_dedicated(6u, 3u);            /* 6 / 2 = 3, exact */
	check_dedicated(7u, 4u);            /* tie 3.5, neighbors 3/4, even 4 */
	check_dedicated(0x80000000u, 0x40000000u); /* 2^31 / 2, exact */
	check_dedicated(0x80000001u, 0x40000000u); /* tie, q = 2^30 even */
	check_dedicated(0x80000003u, 0x40000002u); /* tie, q = 2^30+1 odd */
	check_dedicated(0xFFFFFFFEu, 0x7FFFFFFFu); /* max-1, even, exact q */
	check_dedicated(0xFFFFFFFFu, 0x80000000u); /* max, tie, q odd -> 2^31 */
	if (failures) {
		printf("dedicated rows: %d FAILURES\n", failures);
		return 1;
	}
	printf("ok   dedicated contract rows (13): mismatches 0\n");

	/* Phase 2: exhaustive, all 2^32 inputs. The counter is 64-bit
	 * so the full uint32_t range is covered, including the top. */
	for (uint64_t i = 0; i <= 0xFFFFFFFFULL; i++) {
		uint32_t x = (uint32_t)i;
		uint32_t got = tie_to_even_half(x);
		uint32_t ref = tie_to_even_half_ref(x);
		checksum = fnv1a(checksum, x);
		checksum = fnv1a(checksum, got);
		if (got != ref) {
			if (mismatches < 8) {
				printf("FAIL x=%" PRIu32 " got=%" PRIu32
				       " ref=%" PRIu32 "\n",
				       x, got, ref);
			}
			mismatches++;
		}
	}
	if (mismatches) {
		printf("exhaustive: %" PRIu64 " mismatches\n", mismatches);
		return 1;
	}
	printf("ok   exhaustive u32 (4294967296 values): mismatches 0\n");
	printf("fnv1a over per-case (input, result): 0x%016" PRIx64 "\n",
	       checksum);

	/* Phase 3: throughput at this build's optimization level.
	 * Fixed input array, best of 5; PRNG generation is outside
	 * the timed region. */
	const size_t N = 1 << 22;
	uint32_t *vals = malloc(N * sizeof(*vals));
	if (!vals) {
		perror("malloc");
		return 1;
	}
	uint64_t st = 0x9E3779B97F4A7C15ULL;
	for (size_t i = 0; i < N; i++) {
		st ^= st >> 30; st *= 0xBF58476D1CE4E5B9ULL;
		st ^= st >> 27; st *= 0x94D049BB133111EBULL;
		st ^= st >> 31;
		vals[i] = (uint32_t)(st >> 32);
	}
	double best = 1e18;
	uint64_t sink = 0;
	for (int rep = 0; rep < 5; rep++) {
		struct timespec t0, t1;
		clock_gettime(CLOCK_MONOTONIC, &t0);
		for (size_t i = 0; i < N; i++) {
			sink += tie_to_even_half(vals[i]);
		}
		clock_gettime(CLOCK_MONOTONIC, &t1);
		double ns = (t1.tv_sec - t0.tv_sec) * 1e9 +
		            (t1.tv_nsec - t0.tv_nsec);
		if (ns / N < best) {
			best = ns / N;
		}
	}
	free(vals);
	printf("timed sink (prevents dead-code elimination): %" PRIu64 "\n",
	       sink);
	printf("throughput: %.2f ns/value (best of 5 over %zu values)\n",
	       best, N);

	printf("RESULT: ALL TESTS PASSED\n");
	return 0;
}
