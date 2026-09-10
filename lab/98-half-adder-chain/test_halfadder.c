#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "halfadder.h"

/* ------------------------------------------------------------------ */
/* Independent oracle: the native + operator, plus the native          */
/* overflow identity. The test file is the only place + appears;      */
/* halfadder.c never uses it.                                          */
/* ------------------------------------------------------------------ */

/* Oracle for width w (1..64): exact = a + b + cin computed in widened
 * arithmetic, then masked. The native overflow identity checked on
 * every case is: carry_out == 1 iff a + b + cin wraps out of w bits,
 * which for the native 64-bit + means the classic (t < a) idiom. */
static void oracle_w(uint64_t a, uint64_t b, uint64_t cin, unsigned w,
                     uint64_t *sum, uint64_t *cout)
{
	unsigned __int128 exact = (unsigned __int128)a + b + cin;
	uint64_t mask = (w == 64) ? UINT64_MAX : ((((uint64_t)1 << w) - 1u));

	*sum = ((uint64_t)exact) & mask;
	*cout = (uint64_t)(exact >> w);
}

/* Native 64-bit overflow identity: adding a + b + cin with the native
 * + operator, the true overflow is detected by the (t < a) idiom on
 * each step. This is checked against the chain's carry-out on every
 * case in addition to the widened oracle above. */
static uint64_t native_overflow(uint64_t a, uint64_t b, uint64_t cin)
{
	uint64_t t = a + b;
	uint64_t ov = (t < a);
	uint64_t s = t + (cin & 1u);

	return ov | (s < t);
}

/* splitmix64: fixed-seed PRNG for the random phase. */
static uint64_t splitmix64(uint64_t *state)
{
	uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);

	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

/* FNV-1a 64 over a 64-bit word, for the cross-build checksum. */
static uint64_t fnv1a(uint64_t h, uint64_t v)
{
	h ^= v;
	h *= 0x100000001B3ULL;
	return h;
}

static uint64_t checksum = 0xCBF29CE484222325ULL;
static unsigned long long total_cases = 0;
static unsigned long long mismatches = 0;

/* One case at width w: implementation vs widened oracle, and the
 * chain carry-out vs the native overflow identity. Folds inputs and
 * outputs into the checksum on success, counts a mismatch otherwise. */
static void check_case(uint64_t a, uint64_t b, uint64_t cin, unsigned w)
{
	uint64_t i_sum, i_cout, o_sum, o_cout, nov;

	if (w == 64)
		i_cout = add_carry_chain(a, b, cin, &i_sum);
	else
		i_cout = add_carry_chain_w(a, b, cin, w, &i_sum);
	oracle_w(a, b, cin, w, &o_sum, &o_cout);
	if (w == 64)
		nov = native_overflow(a, b, cin);
	else
		nov = o_cout; /* native idiom is 64-bit; oracle covers w<64 */

	if (i_sum == o_sum && i_cout == o_cout && i_cout == nov) {
		checksum = fnv1a(checksum, a);
		checksum = fnv1a(checksum, b);
		checksum = fnv1a(checksum, cin);
		checksum = fnv1a(checksum, w);
		checksum = fnv1a(checksum, i_sum);
		checksum = fnv1a(checksum, i_cout);
	} else {
		mismatches++;
		if (mismatches <= 5) {
			printf("MISMATCH w=%u a=%016" PRIx64 " b=%016" PRIx64
			       " cin=%" PRIu64 " impl=%016" PRIx64 " c=%" PRIu64
			       " oracle=%016" PRIx64 " c=%" PRIu64
			       " native_ov=%" PRIu64 "\n",
			       w, a, b, cin, i_sum, i_cout,
			       o_sum, o_cout, nov);
		}
	}
	total_cases++;
}

static double now_seconds(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Directed edge rows that pin the carry identity's corner cases at
 * full width 64. */
static void directed_rows(void)
{
	uint64_t one = UINT64_MAX;
	uint64_t alt_a = 0xAAAAAAAAAAAAAAAAULL;
	uint64_t alt_b = 0x5555555555555555ULL;

	check_case(0, 0, 0, 64);
	check_case(0, 0, 1, 64);
	/* all-ones + all-ones: exact sum is 2^65 - 1, resp. 2^65, so the
	 * low word is all-ones and carry-out is 1 in both rows. */
	check_case(one, one, 0, 64);
	check_case(one, one, 1, 64);
	/* Single carry rippling through all 64 positions. */
	check_case(one, 0, 1, 64);   /* 2^64 - 1 + 1 = 2^64: sum 0, cout 1 */
	check_case(one, 1, 0, 64);   /* same, carry_in 0 */
	check_case(one, 1, 1, 64);   /* 2^64 + 1: sum 1, cout 1 */
	check_case(0, one, 1, 64);   /* symmetric to the first row */
	check_case(1, one, 1, 64);
	/* High-bit only operands: carry generated at bit 63. */
	check_case(0x8000000000000000ULL, 0x8000000000000000ULL, 0, 64);
	check_case(0x8000000000000000ULL, 0x8000000000000000ULL, 1, 64);
	check_case(0x8000000000000000ULL, 0x7FFFFFFFFFFFFFFFULL, 0, 64);
	/* Alternating bits: every column sums to exactly 1, so with
	 * carry_in 0 no carry ever forms (sum is all ones, carry-out 0);
	 * with carry_in 1 a single carry ripples through all 64
	 * positions (sum is 0, carry-out 1). */
	check_case(alt_a, alt_b, 0, 64);
	check_case(alt_a, alt_b, 1, 64);
	/* Carry chain that dies exactly at the top bit: bit 63 all ones
	 * plus a low word that just reaches bit 63. */
	check_case(0x000000000000FFFFULL, 0xFFFFFFFFFFFF0001ULL, 0, 64);
	check_case(0x0000000000000001ULL, 0x7FFFFFFFFFFFFFFFULL, 1, 64);
	/* Sparse one-bit operands across the word, both carry values. */
	{
		unsigned i;

		for (i = 0; i < 64; i += 7) {
			check_case((uint64_t)1 << i, (uint64_t)1 << i, 0, 64);
			check_case((uint64_t)1 << i, (uint64_t)1 << i, 1, 64);
		}
	}
	/* A value that is one below a power of two plus one: long run of
	 * ones flipped by a single carry, ending mid-word. */
	check_case(0x0000000000FFFFFFULL, 0x0000000000000001ULL, 0, 64);
	check_case(0x00000000FFFFFFFFULL, 0x0000000000000001ULL, 1, 64);

	printf("directed edge rows: mismatches so far %llu\n",
	       (unsigned long long)mismatches);
}

/* Exhaustive over all 16-bit (a, b) pairs x carry_in 0/1 at width 16:
 * 2^33 cases. Every 16-bit addition, including every possible carry
 * ripple length (0 through 16), is seen. */
static void exhaustive_16bit(void)
{
	double t0 = now_seconds();
	uint32_t a, b;
	unsigned cin;

	for (a = 0; a < 0x10000u; a++) {
		for (b = 0; b < 0x10000u; b++) {
			for (cin = 0; cin < 2; cin++)
				check_case(a, b, cin, 16);
		}
	}

	printf("exhaustive 16-bit pairs x carry_in (8589934592 cases): %.1f s wall\n",
	       now_seconds() - t0);
}

/* 1,000,000 fixed-seed splitmix64 random 64-bit pairs, carry_in
 * alternating 0/1. */
static void random_phase(void)
{
	uint64_t state = 0x123456789ABCDEF0ULL;
	int i;

	for (i = 0; i < 1000000; i++) {
		uint64_t a = splitmix64(&state);
		uint64_t b = splitmix64(&state);
		uint64_t cin = (uint64_t)(i & 1);

		check_case(a, b, cin, 64);
	}

	printf("random 64-bit pairs: 1000000 (seed 0x123456789ABCDEF0, carry_in alternating)\n");
}

/* Throughput at the timed build: 1,000,000 pre-generated 64-bit cases,
 * PRNG outside the timed region, best of 5, sink against DCE. */
static void timed_phase(void)
{
	uint64_t *va, *vb, *vcin;
	uint64_t state = 0xDEADBEEFCAFEBABEULL;
	uint64_t sink = 0;
	double best = 1e300;
	int r, i;

	va = malloc(1000000 * sizeof(*va));
	vb = malloc(1000000 * sizeof(*vb));
	vcin = malloc(1000000 * sizeof(*vcin));
	if (!va || !vb || !vcin) {
		printf("timed_phase: allocation failed\n");
		exit(1);
	}

	for (i = 0; i < 1000000; i++) {
		va[i] = splitmix64(&state);
		vb[i] = splitmix64(&state);
		vcin[i] = (uint64_t)(i & 1);
	}

	for (r = 0; r < 5; r++) {
		double t0 = now_seconds();

		for (i = 0; i < 1000000; i++) {
			uint64_t s;
			uint64_t c = add_carry_chain(va[i], vb[i], vcin[i], &s);

			sink += s + c;
		}
		{
			double dt = now_seconds() - t0;

			if (dt < best)
				best = dt;
		}
	}

	free(va);
	free(vb);
	free(vcin);

	printf("timed sink (prevents dead-code elimination): %" PRIu64 "\n",
	       sink);
	printf("throughput: %.2f ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)\n",
	       best / 1000000.0 * 1e9);
}

int main(void)
{
	directed_rows();
	exhaustive_16bit();
	random_phase();
	timed_phase();

	printf("total cases: %llu\n", total_cases);
	printf("implementation vs native-+ oracle mismatches (all phases): %llu\n",
	       (unsigned long long)mismatches);
	printf("fnv1a over per-case (inputs, outputs): 0x%016" PRIx64 "\n",
	       checksum);
	if (mismatches == 0) {
		printf("RESULT: ALL TESTS PASSED\n");
		return 0;
	}
	printf("RESULT: FAILURES PRESENT\n");
	return 1;
}
