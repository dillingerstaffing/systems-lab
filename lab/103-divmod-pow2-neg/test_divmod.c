/*
 * Differential test for divmod_pow2 (lab/103-divmod-pow2-neg).
 *
 * Domain: every k = 1..63 against
 *   (a) all 65,536 int16_t inputs, exhaustively, and
 *   (b) 10 fixed 64-bit edge values per k (INT64_MAX, INT64_MIN + 1,
 *       -1, 0, 1, large alternating bit patterns, a large odd value
 *       and its negation).
 * Reference: C's / and % on a signed int64_t divisor 2^k. The / and %
 * operators appear only in the reference path, never in divmod.c.
 *
 * Checks per (x, k): q == qref, r == rref, plus the invariants
 * q * 2^k + r == x (in __int128, exact), r == 0 or sign(r) == sign(x),
 * and |r| < 2^k (exact unsigned magnitude compare).
 *
 * An FNV-1a checksum over every (q, r) pair is printed so builds can
 * be compared bit for bit. A second pass times throughput at -O2.
 */
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "divmod.h"

static uint64_t fnv1a(uint64_t h, uint64_t v)
{
	for (int i = 0; i < 8; i++) {
		h ^= (v >> (i * 8)) & 0xffu;
		h *= 0x100000001b3ull;
	}
	return h;
}

static uint64_t uabs64(int64_t v)
{
	/* exact for all v != INT64_MIN, which the contract excludes */
	return v < 0 ? (uint64_t)(-(v + 1)) + 1u : (uint64_t)v;
}

static const int64_t edge_vals[] = {
	INT64_MAX,
	INT64_MIN + 1,
	-1, 0, 1,
	INT64_MAX >> 1,
	-(INT64_MAX >> 1),
	(int64_t)0x5555555555555555ull,
	(int64_t)0xaaaaaaaaaaaaaaaaull,
	1234567890123456789ll,
	-1234567890123456789ll,
};

static int check_one(int64_t x, unsigned k, uint64_t *h, unsigned long *n)
{
	int64_t q, r;
	int64_t d = (int64_t)((uint64_t)1 << k); /* 2^k, signed; k=63 gives INT64_MIN */
	int64_t qref, rref;

	divmod_pow2(x, k, &q, &r);
	qref = x / d;
	rref = x % d;

	if (q != qref || r != rref)
		return 0;

	/* invariant: q * 2^k + r == x, exact in 128 bits */
	__int128 rebuild = (__int128)q * (__int128)d + (__int128)r;
	if (rebuild != (__int128)x)
		return 0;

	/* invariant: r is 0 or carries the sign of x (C % semantics) */
	if (r != 0 && (r > 0) != (x > 0))
		return 0;

	/* invariant: |r| < 2^k */
	if (!(uabs64(r) < ((uint64_t)1 << k)))
		return 0;

	*h = fnv1a(*h, (uint64_t)q);
	*h = fnv1a(*h, (uint64_t)r);
	(*n)++;
	return 1;
}

int main(void)
{
	uint64_t h = 0xcbf29ce484222325ull;
	unsigned long n = 0;
	unsigned long bad = 0;
	struct timespec t0, t1;

	for (unsigned k = 1; k <= 63; k++) {
		for (int32_t xi = -32768; xi <= 32767; xi++) {
			if (!check_one((int64_t)xi, k, &h, &n)) {
				if (bad < 5)
					printf("MISMATCH x=%d k=%u\n", xi, k);
				bad++;
			}
		}
		for (size_t i = 0; i < sizeof(edge_vals) / sizeof(edge_vals[0]); i++) {
			if (!check_one(edge_vals[i], k, &h, &n)) {
				if (bad < 5)
					printf("MISMATCH x=%lld k=%u\n",
					       (long long)edge_vals[i], k);
				bad++;
			}
		}
	}

	printf("checked=%lu mismatches=%lu fnv1a=%016llx\n",
	       n, bad, (unsigned long long)h);

	/* throughput: same domain, timed, checksum kept so it is not dead code */
	clock_gettime(CLOCK_MONOTONIC, &t0);
	uint64_t h2 = 0xcbf29ce484222325ull;
	unsigned long n2 = 0;
	for (unsigned k = 1; k <= 63; k++) {
		for (int32_t xi = -32768; xi <= 32767; xi++) {
			int64_t q, r;
			divmod_pow2((int64_t)xi, k, &q, &r);
			h2 = fnv1a(h2, (uint64_t)q);
			h2 = fnv1a(h2, (uint64_t)r);
			n2++;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	double ns = (t1.tv_sec - t0.tv_sec) * 1e9 + (t1.tv_nsec - t0.tv_nsec);
	printf("timed_values=%lu total_ns=%.0f ns_per_value=%.2f fnv1a=%016llx\n",
	       n2, ns, ns / (double)n2, (unsigned long long)h2);

	return bad == 0 ? 0 : 1;
}
