/*
 * test_xor_swap.c: differential verification of xor_swap against a
 * temporary-variable reference swap.
 *
 * Checks:
 *   1. Aliasing contract: xor_swap(&x, &x) must zero x (dedicated rows).
 *   2. Exhaustive: all 2^32 ordered pairs of 16-bit values (held as
 *      uint64_t), requiring new_a == old_b and new_b == old_a on every
 *      case, 0 mismatches.
 *   3. Random: 1,000,000 splitmix64 pairs (fixed seed 0x123456789ABCDEF0,
 *      the same seed used by sibling labs), same per-case assertion.
 *   4. FNV-1a (64-bit) folded over every output word, printed so builds
 *      can be cross-checked for identical behavior.
 *   5. Throughput: best-of-5 over 100M pairs at whatever -O level this
 *      binary was built with, via a volatile function pointer so the
 *      call cannot be inlined or optimized away.
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "xor_swap.h"

#define RANDOM_PAIRS 1000000u
#define TIMING_PAIRS 100000000u
#define TIMING_RUNS 5

static uint64_t splitmix64(uint64_t *s)
{
	uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

static void ref_swap(uint64_t *a, uint64_t *b)
{
	uint64_t t = *a;
	*a = *b;
	*b = t;
}

static uint64_t fnv = 14695981039346656037ULL;

static void fnv_add_u64(uint64_t v)
{
	unsigned i;
	for (i = 0; i < 8; i++) {
		fnv ^= (uint8_t)(v >> (i * 8));
		fnv *= 1099511628211ULL;
	}
}

static uint64_t mismatches;
static uint64_t cases;

static void check_one(uint64_t oa, uint64_t ob)
{
	uint64_t a = oa, b = ob;    /* under test */
	uint64_t ea = oa, eb = ob;  /* temp-variable reference */
	ref_swap(&ea, &eb);
	xor_swap(&a, &b);
	cases++;
	if (a != ea || b != eb) {
		if (mismatches < 8)
			printf("MISMATCH a=%016" PRIx64 " b=%016" PRIx64
			       " -> xor=%016" PRIx64 ":%016" PRIx64
			       " ref=%016" PRIx64 ":%016" PRIx64 "\n",
			       oa, ob, a, b, ea, eb);
		mismatches++;
	}
	fnv_add_u64(a);
	fnv_add_u64(b);
}

static double now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

int main(void)
{
	uint64_t i, j, n;
	uint64_t state;
	uint64_t alias_cases = 0;
	static const uint64_t alias_vals[] = {
		0x0000000000000000ULL, 0x0000000000000001ULL,
		0x8000000000000000ULL, 0xFFFFFFFFFFFFFFFFULL,
		0x123456789ABCDEF0ULL, 0xDEADBEEFCAFEBABEULL
	};
	void (*volatile fp)(uint64_t *, uint64_t *) = xor_swap;

	/* 1. Aliasing contract: same address zeroes the word. */
	for (n = 0; n < sizeof(alias_vals) / sizeof(alias_vals[0]); n++) {
		uint64_t v = alias_vals[n];
		xor_swap(&v, &v);
		alias_cases++;
		if (v != 0) {
			printf("ALIAS-FAIL input=%016" PRIx64 " got=%016" PRIx64 "\n",
			       alias_vals[n], v);
			mismatches++;
		}
		printf("alias row: input=%016" PRIx64 " -> output=%016" PRIx64 "%s\n",
		       alias_vals[n], v, v == 0 ? " (zeroed, contract holds)" : "");
	}

	/* 2. Exhaustive 2^32 pairs of 16-bit values as uint64_t. */
	for (i = 0; i < 65536; i++)
		for (j = 0; j < 65536; j++)
			check_one(i, j);
	printf("exhaustive 16-bit pairs: done\n");

	/* 3. 1,000,000 fixed-seed random 64-bit pairs. */
	state = 0x123456789ABCDEF0ULL;
	for (n = 0; n < RANDOM_PAIRS; n++)
		check_one(splitmix64(&state), splitmix64(&state));
	printf("random pairs: done\n");

	/* 4. Throughput: best of 5 runs over TIMING_PAIRS pairs. */
	{
		double best = 0.0;
		int r;
		volatile uint64_t sink = 0;
		for (r = 0; r < TIMING_RUNS; r++) {
			double t0 = now_ns();
			for (n = 0; n < TIMING_PAIRS; n++) {
				uint64_t a = (uint64_t)n * 0x9E3779B97F4A7C15ULL;
				uint64_t b = a ^ 0xBF58476D1CE4E5B9ULL;
				fp(&a, &b);
				sink += a + b;
			}
			double dt = now_ns() - t0;
			double per = dt / (double)TIMING_PAIRS;
			if (r == 0 || per < best)
				best = per;
			printf("timing run %d: %.3f ns/pair\n", r, per);
		}
		printf("timing best: %.3f ns/pair (%u pairs x %d runs)\n",
		       best, TIMING_PAIRS, TIMING_RUNS);
		printf("timing sink: %016" PRIx64 " (prevents DCE)\n", sink);
	}

	printf("alias rows: %" PRIu64 "\n", alias_cases);
	printf("differential cases: %" PRIu64 "\n", cases);
	printf("mismatches: %" PRIu64 "\n", mismatches);
	printf("FNV-1a over outputs: %016" PRIx64 "\n", fnv);

	return mismatches == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
