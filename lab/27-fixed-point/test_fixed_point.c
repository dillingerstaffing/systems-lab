#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "fixed_point.h"

/* Fixed-seed splitmix64 PRNG: every run generates the identical
 * sequence, so the test is fully reproducible. Seed is fixed below. */
static uint64_t rng_state;

static uint64_t rng_next(void)
{
	uint64_t z = (rng_state += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

static double now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* Independent oracle for add: the exact sum in int64_t (which cannot
 * overflow for 32-bit operands), narrowed with defined wraparound. */
static int32_t add_oracle(int32_t a, int32_t b)
{
	return (int32_t)(uint32_t)((int64_t)a + (int64_t)b);
}

/*
 * Independent oracle for mul. Uses division and remainder instead of
 * the shift-and-add of the implementation, so the two agree only if
 * the rounding rule is right. Rounds to nearest, ties away from zero;
 * narrows with defined wraparound. Also reports, through the out
 * params, whether the exact rounded product fit in int32_t and whether
 * the product sat exactly on a rounding tie.
 */
static int32_t mul_oracle(int32_t a, int32_t b, int *in_range, int *tie)
{
	int64_t p = (int64_t)a * (int64_t)b; /* exact: |p| <= 2^62 */
	int64_t q = p / 65536;              /* truncates toward zero */
	int64_t r = p % 65536;              /* sign follows p */
	int64_t ar = r < 0 ? -r : r;        /* 0 <= ar <= 65535 */

	*tie = (2 * ar == 65536);
	if (2 * ar >= 65536)
		q += (p >= 0) ? 1 : -1;     /* round up in magnitude; ties go here */

	*in_range = (q >= INT32_MIN && q <= INT32_MAX);
	return (int32_t)(uint32_t)q;
}

static uint64_t checksum;

/* Fold one 32-bit result into the running checksum (splitmix64-style
 * mix); identical checksums across builds prove identical behavior. */
static void fold(int32_t v)
{
	checksum ^= (uint64_t)(uint32_t)v + 0x9E3779B97F4A7C15ULL
		+ (checksum << 6) + (checksum >> 2);
}

static uint64_t mismatches;
static uint64_t mul_in_range_checks;  /* mul cases checked vs double ref */
static uint64_t mul_wrap_checks;      /* mul cases checked vs wrap oracle */
static uint64_t mul_ties_seen;        /* exact rounding ties observed */
static uint64_t add_checks;

#define ULP (1.0 / 65536.0)

static void check_mul(int32_t a, int32_t b)
{
	int in_range, tie;
	int32_t got = q16_mul(a, b);
	int32_t exp = mul_oracle(a, b, &in_range, &tie);

	fold(got);
	if (got != exp) {
		mismatches++;
		if (mismatches < 5)
			printf("MUL MISMATCH a=%" PRId32 " b=%" PRId32
			       " got=%" PRId32 " exp=%" PRId32 "\n",
			       a, b, got, exp);
		return;
	}

	if (in_range) {
		/* Double reference: a/65536 and b/65536 are exactly
		 * representable, and their product needs at most 64 bits,
		 * so the double is within ~2^-53 relative of exact. The
		 * rounded Q16.16 result is within 0.5 ulp of exact, so
		 * 1 ulp of slack is generous. */
		double d = ((double)a / 65536.0) * ((double)b / 65536.0);
		double rv = (double)got / 65536.0;
		mul_in_range_checks++;
		if (fabs(rv - d) > ULP) {
			mismatches++;
			if (mismatches < 5)
				printf("MUL DOUBLE-DRIFT a=%" PRId32 " b=%" PRId32
				       " err=%.9f ulp\n", a, b, fabs(rv - d) / ULP);
		}
		if (tie)
			mul_ties_seen++;
	} else {
		/* Out of range: result must equal the wrapped oracle, and
		 * the true product must really be out of range. */
		double d = ((double)a / 65536.0) * ((double)b / 65536.0);
		mul_wrap_checks++;
		if (fabs(d) < 32767.0) {
			mismatches++;
			printf("MUL RANGE-PATH a=%" PRId32 " b=%" PRId32 "\n", a, b);
		}
	}
}

static void check_add(int32_t a, int32_t b)
{
	int32_t got = q16_add(a, b);
	int32_t exp = add_oracle(a, b);

	fold(got);
	add_checks++;
	if (got != exp) {
		mismatches++;
		if (mismatches < 5)
			printf("ADD MISMATCH a=%" PRId32 " b=%" PRId32
			       " got=%" PRId32 " exp=%" PRId32 "\n",
			       a, b, got, exp);
		return;
	}

	/*
	 * Double reference with wraparound: da and db are exact, and
	 * |da + db| < 65536 needs at most 33 bits, so the double sum d
	 * is exact. The Q16.16 sum is exact too; on overflow it wraps
	 * mod 2^32 in raw units, i.e. the result value is congruent to
	 * d modulo 65536. Every operation below is exact in double
	 * (powers of two, values under 2^53), so the residual is
	 * exactly 0.0 when the congruence holds.
	 */
	double d = (double)a / 65536.0 + (double)b / 65536.0;
	double rv = (double)got / 65536.0;
	double diff = rv - d;
	double k = round(diff / 65536.0);
	if (diff - 65536.0 * k != 0.0) {
		mismatches++;
		if (mismatches < 5)
			printf("ADD DOUBLE-DRIFT a=%" PRId32 " b=%" PRId32 "\n", a, b);
	}
}

/* Interesting values: range edges, wraparound corners, exact
 * rounding ties (raw product fractional part == 0x8000), and small
 * integers. The cross product is exercised for both ops. */
static const int32_t interesting[] = {
	0, 1, -1, 2, -2, 3, -3,
	32767, 32768, -32768, 65535, 65536, -65536, 98304, -98304,
	0x00010000, 0x00008000, -0x00008000,
	INT32_MAX, INT32_MAX - 1, INT32_MIN, INT32_MIN + 1,
	0x7FFF0000, -0x7FFF0000, 0x0000FFFF, -0x0000FFFF,
};

int main(void)
{
	size_t n = sizeof(interesting) / sizeof(interesting[0]);
	size_t i, j;
	uint64_t directed = 0;
	double t0, t1;

	rng_state = 0x9E3779B97F4A7C15ULL; /* stated seed */

	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			check_mul(interesting[i], interesting[j]);
			check_add(interesting[i], interesting[j]);
			directed += 2;
		}
	}
	printf("directed=%" PRIu64 "\n", directed);

	/* 5M random mul pairs: 70% small-small (products stay in range,
	 * exercising the double-reference path), 15% mixed, 15%
	 * full-range (exercising documented wraparound). */
	t0 = now_ns();
	for (i = 0; i < 5000000; i++) {
		uint64_t pick = rng_next() % 100;
		int32_t a, b;
		if (pick < 70) {
			a = (int32_t)(rng_next() & 0x1FFFFFU) - 0x100000;
			b = (int32_t)(rng_next() & 0x1FFFFFU) - 0x100000;
		} else if (pick < 85) {
			a = (int32_t)(rng_next() & 0x1FFFFFU) - 0x100000;
			b = (int32_t)rng_next();
		} else {
			a = (int32_t)rng_next();
			b = (int32_t)rng_next();
		}
		check_mul(a, b);
	}
	t1 = now_ns();
	printf("mul: pairs=5000000 ns_total=%.0f ns_per_op=%.2f\n",
	       t1 - t0, (t1 - t0) / 5000000.0);

	/* 5M random add pairs: full 32-bit range; the wrap oracle and the
	 * exact double congruence cover every case. */
	t0 = now_ns();
	for (i = 0; i < 5000000; i++) {
		int32_t a = (int32_t)rng_next();
		int32_t b = (int32_t)rng_next();
		check_add(a, b);
	}
	t1 = now_ns();
	printf("add: pairs=5000000 ns_total=%.0f ns_per_op=%.2f\n",
	       t1 - t0, (t1 - t0) / 5000000.0);

	printf("mul_in_range=%" PRIu64 " mul_wrap=%" PRIu64
	       " mul_ties=%" PRIu64 " add=%" PRIu64 "\n",
	       mul_in_range_checks, mul_wrap_checks, mul_ties_seen, add_checks);
	printf("total_cases=%" PRIu64 " mismatches=%" PRIu64
	       " checksum=%" PRIu64 "\n",
	       directed + (uint64_t)10000000, mismatches, checksum);

	if (mismatches == 0) {
		printf("PASS\n");
		return 0;
	}
	printf("FAIL\n");
	return 1;
}
