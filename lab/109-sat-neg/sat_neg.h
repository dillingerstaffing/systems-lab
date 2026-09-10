#ifndef SAT_NEG_H
#define SAT_NEG_H

#include <stdint.h>

/*
 * neg_sat64(x): saturating negation of an int64_t.
 * Returns INT64_MAX when x == INT64_MIN, and -x otherwise.
 *
 * From the sign-mask identity, with the INT64_MIN edge folded
 * into the mask so no branch and no comparison is needed:
 *
 *   s = (uint64_t)(x >> 63)        sign mask: all ones iff x < 0
 *   d = u ^ 0x8000000000000000     zero iff x == INT64_MIN
 *   e = 1 - ((d | (0 - d)) >> 63)  one iff x == INT64_MIN
 *   neg_sat64(x) = s - u + ((s & 1) ^ e)
 *
 * All arithmetic is unsigned (mod 2^64); the arithmetic right
 * shift is the only implementation-defined operation and is
 * sign-extending on every two's complement target.
 */
int64_t neg_sat64(int64_t x);

#endif
