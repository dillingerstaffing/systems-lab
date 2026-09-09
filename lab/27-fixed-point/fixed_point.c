#include "fixed_point.h"

int32_t q16_add(int32_t a, int32_t b)
{
	/* Unsigned addition is defined to wrap mod 2^32; the bit pattern
	 * of a Q16.16 sum is exactly the wrapped integer sum of the raw
	 * patterns. The int64_t-free form keeps the whole operation in
	 * 32 bits, where every step is defined. */
	return (int32_t)((uint32_t)a + (uint32_t)b);
}

int32_t q16_mul(int32_t a, int32_t b)
{
	/* Exact product: |a| <= 2^31 and |b| <= 2^31, so
	 * |product| <= 2^62 < INT64_MAX. No overflow is possible here. */
	int64_t p = (int64_t)a * (int64_t)b;
	int64_t q;

	if (p >= 0) {
		/* (p + 32768) <= 2^62 + 32768 < 2^63: no overflow.
		 * Shift of a non-negative value: defined. */
		q = (p + 32768) >> 16;
	} else {
		/* -p <= 2^62, so (-p + 32768) < 2^63: no overflow.
		 * Shift of a non-negative value: defined.
		 * Negating the rounded magnitude rounds ties in the
		 * negative half away from zero, mirroring the
		 * non-negative branch. */
		q = -((-p + 32768) >> 16);
	}

	/* Narrowing through uint32_t is defined to wrap mod 2^32:
	 * a rounded product outside the representable range wraps. */
	return (int32_t)(uint32_t)q;
}
