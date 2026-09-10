#ifndef DIVMOD_POW2_NEG_H
#define DIVMOD_POW2_NEG_H

#include <stdint.h>

/*
 * divmod_pow2: quotient and remainder of x divided by 2^k, one pass.
 *
 * Contract:
 *   - k in [1, 63]. k = 0 would be the identity; k = 64 is not
 *     representable as a shift width, so both are outside the contract.
 *   - x != INT64_MIN. INT64_MIN has no representable negation, and the
 *     sign-corrected division identity is contracted over the domain
 *     where |x| is representable.
 *   - r is zero or carries the sign of x (C semantics); |r| < 2^k;
 *     q * 2^k + r == x.
 *
 * The body contains no / or % operators; see divmod.c for the two
 * identities that produce q and r.
 */
void divmod_pow2(int64_t x, unsigned k, int64_t *q, int64_t *r);

#endif
