#include "divmod.h"

void divmod_pow2(int64_t x, unsigned k, int64_t *q, int64_t *r)
{
	/*
	 * Quotient identity: on this target, >> of a negative value is an
	 * arithmetic shift, which rounds toward negative infinity. C /
	 * rounds toward zero, so a negative x needs a positive bias of
	 * (2^k - 1) before the shift. x >> 63 is 0 for x >= 0 and all
	 * ones (i.e. -1) for x < 0, so the AND selects the bias exactly
	 * when x is negative.
	 *
	 * q = (x + ((x >> 63) & (2^k - 1))) >> k
	 *
	 * For k = 63, ((uint64_t)1 << 63) - 1 is representable, so the
	 * mask shift is exact. The bias addition cannot overflow under
	 * the contract: for x >= 0 the bias is 0; for x < 0 the bias
	 * is 2^k - 1 with x <= -1, giving x + bias <= 2^k - 2
	 * (<= INT64_MAX - 1), and x >= INT64_MIN + 1 keeps the sum
	 * >= INT64_MIN.
	 */
	uint64_t mask = ((uint64_t)1 << k) - 1;
	int64_t bias = (int64_t)(((uint64_t)(x >> 63)) & mask);
	int64_t qq = (x + bias) >> k;

	/*
	 * Remainder identity: r = x - (q << k). This reconstructs the
	 * truncated multiple from the quotient and subtracts it back
	 * out. No division is used; the sign of r follows x because q
	 * was rounded toward zero. The shift of a negative q would be
	 * undefined behavior in the signed domain, so the multiply is
	 * done in unsigned arithmetic: (uint64_t)q << k is exact
	 * modulo 2^64, the subtraction wraps exactly, and the true r
	 * lies in (-2^k, 2^k), hence is representable, so re-reading
	 * the bits as int64_t yields exactly r.
	 */
	uint64_t uprod = (uint64_t)qq << k;
	uint64_t urr = (uint64_t)x - uprod;
	int64_t rr = (int64_t)urr;

	*q = qq;
	*r = rr;
}
