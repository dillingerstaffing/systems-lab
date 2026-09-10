#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "subborrow.h"

/* ------------------------------------------------------------------ */
/* Independent oracle: unsigned __int128 exact arithmetic. The test    */
/* file is the only place __int128 appears; subborrow.c never uses it. */
/* ------------------------------------------------------------------ */

typedef unsigned __int128 u128;

static void oracle(uint64_t a_hi, uint64_t a_lo,
                   uint64_t b_hi, uint64_t b_lo,
                   uint64_t bin,
                   uint64_t *diff_hi, uint64_t *diff_lo,
                   uint64_t *borrow_out)
{
	u128 A = ((u128)a_hi << 64) | a_lo;
	u128 B = ((u128)b_hi << 64) | b_lo;

	u128 y = B + bin; /* wraps mod 2^128 */
	/* borrow-out is 1 iff A < B + bin in exact arithmetic. If the
	 * 128-bit addition of bin wrapped, then B == 2^128 - 1 and
	 * B + bin == 2^128 exactly, which every A is below. Otherwise
	 * y is exact and the borrow test is (A < y). */
	uint64_t carry = (bin && (y < B));
	uint64_t borrow = carry ? 1u : (A < y);

	u128 d = A - y; /* wraps mod 2^128, equals A - B - bin mod 2^128 */
	*diff_lo = (uint64_t)d;
	*diff_hi = (uint64_t)(d >> 64);
	*borrow_out = borrow;
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

/* One case: implementation vs oracle; folds inputs and outputs into the
 * checksum on success, counts a mismatch otherwise. */
static void check_case(uint64_t a_hi, uint64_t a_lo,
                       uint64_t b_hi, uint64_t b_lo,
                       uint64_t bin)
{
	uint64_t i_lo, i_hi, o_lo, o_hi, i_b, o_b;

	i_b = sub_borrow_chain(a_hi, a_lo, b_hi, b_lo, bin, &i_hi, &i_lo);
	oracle(a_hi, a_lo, b_hi, b_lo, bin, &o_hi, &o_lo, &o_b);

	if (i_lo == o_lo && i_hi == o_hi && i_b == o_b) {
		checksum = fnv1a(checksum, a_hi);
		checksum = fnv1a(checksum, a_lo);
		checksum = fnv1a(checksum, b_hi);
		checksum = fnv1a(checksum, b_lo);
		checksum = fnv1a(checksum, bin);
		checksum = fnv1a(checksum, i_lo);
		checksum = fnv1a(checksum, i_hi);
		checksum = fnv1a(checksum, i_b);
	} else {
		mismatches++;
		if (mismatches <= 5) {
			printf("MISMATCH a=%016" PRIx64 "%016" PRIx64
			       " b=%016" PRIx64 "%016" PRIx64 " bin=%" PRIu64
			       " impl=%016" PRIx64 "%016" PRIx64 " b=%" PRIu64
			       " oracle=%016" PRIx64 "%016" PRIx64 " b=%" PRIu64 "\n",
			       a_hi, a_lo, b_hi, b_lo, bin,
			       i_hi, i_lo, i_b, o_hi, o_lo, o_b);
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

/* Directed edge rows that pin the borrow identity's corner cases. */
static void directed_rows(void)
{
	uint64_t one = UINT64_MAX;
	int i;

	/* b_lo == UINT64_MAX with borrow_in 1: t wraps to 0, borrow_lo
	 * must be 1 for every a_lo. */
	for (i = 0; i < 8; i++) {
		uint64_t a_lo = (uint64_t)(i * 0x0101010101010101ULL);
		check_case(0, a_lo, 0, one, 1);
		check_case(0, a_lo, 0, one, 0);
	}
	/* a_lo == 0 and b_lo == 0 rows, both borrow values. */
	check_case(0, 0, 0, 0, 0);
	check_case(0, 0, 0, 0, 1);
	check_case(one, one, 0, 0, 0);
	check_case(one, one, 0, 0, 1);
	/* All-ones words, both borrow values. */
	check_case(one, one, one, one, 0);
	check_case(one, one, one, one, 1);
	/* borrow_in 1 with a == b: diff must be all-ones words,
	 * borrow-out 1 (exact difference is -1). */
	check_case(0x123456789ABCDEF0ULL, 0x0FEDCBA987654321ULL,
	           0x123456789ABCDEF0ULL, 0x0FEDCBA987654321ULL, 1);
	check_case(0, 0, 0, 0, 1);
	check_case(one, one, one, one, 1);
	/* a == 0 with borrow_in 1: diff is -b-1 mod 2^128, borrow-out
	 * is 1 unless b+1 wraps (b all ones) ... checked against oracle. */
	check_case(0, 0, 0, 0, 1);
	check_case(0, 0, one, one, 1);
	check_case(0, 0, 0x8000000000000000ULL, 0x8000000000000000ULL, 1);
	/* Borrow chaining across the word boundary: a_lo < b_lo with
	 * a_hi just big enough to absorb it, and the symmetric case. */
	check_case(1, 0, 0, 1, 0);          /* 2^64 - 1 */
	check_case(0, 0, 0, 1, 0);          /* -1 mod 2^128, borrow 1 */
	check_case(one, 5, one, 3, 1);      /* borrow_in cancels */
	check_case(0, 0, 0, 0, 1);          /* double subtraction of 1 */
	/* b_hi == UINT64_MAX with low-stage borrow 1: the wrap detector
	 * fires on the high stage too. */
	check_case(0, 0, one, 1, 0);
	check_case(0, 0, one, 0, 1);
	/* Same high words, mixed low words, both borrow values. */
	check_case(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL,
	           0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL, 0);
	check_case(0xAAAAAAAAAAAAAAAAULL, 0x5555555555555555ULL,
	           0x5555555555555555ULL, 0xAAAAAAAAAAAAAAAAULL, 1);

	printf("directed edge rows: mismatches so far %" PRIu64 "\n",
	       (uint64_t)mismatches);
}

/* Exhaustive 16-bit (a_lo, b_lo) pairs x borrow_in 0/1 with high
 * words 0: 2^33 cases. The low stage sees every possible 16-bit
 * subtraction, including every b_lo wrap case at the 16-bit level;
 * the high stage sees a_hi = b_hi = 0 with borrow_lo 0/1, which pins
 * the (0 - 0 - borrow_lo) row and the final borrow-out identity. */
static void exhaustive_16bit(void)
{
	double t0 = now_seconds();
	uint32_t a, b;
	int bin;

	for (a = 0; a < 0x10000u; a++) {
		for (b = 0; b < 0x10000u; b++) {
			for (bin = 0; bin < 2; bin++) {
				check_case(0, a, 0, b, (uint64_t)bin);
			}
		}
	}

	printf("exhaustive 16-bit x borrow_in (8589934592 cases): %.1f s wall\n",
	       now_seconds() - t0);
}

/* 10,000,000 fixed-seed splitmix64 random 128-bit cases,
 * borrow_in alternating 0/1. */
static void random_phase(void)
{
	uint64_t state = 0x123456789ABCDEF0ULL;
	int i;

	for (i = 0; i < 10000000; i++) {
		uint64_t a_hi = splitmix64(&state);
		uint64_t a_lo = splitmix64(&state);
		uint64_t b_hi = splitmix64(&state);
		uint64_t b_lo = splitmix64(&state);
		uint64_t bin = (uint64_t)(i & 1);

		check_case(a_hi, a_lo, b_hi, b_lo, bin);
	}

	printf("random 64-bit cases: 10000000 (seed 0x123456789ABCDEF0, borrow_in alternating)\n");
}

/* Throughput at the timed build: 1,000,000 pre-generated cases,
 * PRNG outside the timed region, best of 5, sink against DCE. */
static void timed_phase(void)
{
	uint64_t *va_hi, *va_lo, *vb_hi, *vb_lo, *vbin;
	uint64_t state = 0xDEADBEEFCAFEBABEULL;
	uint64_t sink = 0;
	double best = 1e300;
	int r, i;

	va_hi = malloc(1000000 * sizeof(*va_hi));
	va_lo = malloc(1000000 * sizeof(*va_lo));
	vb_hi = malloc(1000000 * sizeof(*vb_hi));
	vb_lo = malloc(1000000 * sizeof(*vb_lo));
	vbin = malloc(1000000 * sizeof(*vbin));
	if (!va_hi || !va_lo || !vb_hi || !vb_lo || !vbin) {
		printf("timed_phase: allocation failed\n");
		exit(1);
	}

	for (i = 0; i < 1000000; i++) {
		va_hi[i] = splitmix64(&state);
		va_lo[i] = splitmix64(&state);
		vb_hi[i] = splitmix64(&state);
		vb_lo[i] = splitmix64(&state);
		vbin[i] = (uint64_t)(i & 1);
	}

	for (r = 0; r < 5; r++) {
		double t0 = now_seconds();

		for (i = 0; i < 1000000; i++) {
			uint64_t d_hi, d_lo;

			sink += sub_borrow_chain(va_hi[i], va_lo[i],
			                         vb_hi[i], vb_lo[i],
			                         vbin[i], &d_hi, &d_lo)
			        + d_hi + d_lo;
		}

		{
			double dt = now_seconds() - t0;
			double ns = dt * 1e9 / 1000000.0;

			if (ns < best) {
				best = ns;
			}
		}
	}

	printf("timed sink (prevents dead-code elimination): %" PRIu64 "\n", sink);
	printf("throughput: %.2f ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)\n",
	       best);

	free(va_hi);
	free(va_lo);
	free(vb_hi);
	free(vb_lo);
	free(vbin);
}

int main(void)
{
	directed_rows();
	exhaustive_16bit();
	random_phase();
	timed_phase();

	printf("total cases: %llu\n", total_cases);
	printf("implementation vs __int128 oracle mismatches (all phases): %llu\n",
	       mismatches);
	printf("fnv1a over per-case (inputs, outputs): 0x%016" PRIx64 "\n",
	       checksum);
	printf("RESULT: %s\n", mismatches == 0 ? "ALL TESTS PASSED" : "FAILURES PRESENT");

	return mismatches == 0 ? 0 : 1;
}
