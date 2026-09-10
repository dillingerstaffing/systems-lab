#ifndef CMP_100_H
#define CMP_100_H

#include <stdint.h>

/*
 * bcmp64: branchless three-way unsigned comparison.
 *
 * Returns -1 if a < b, 0 if a == b, +1 if a > b, computed from the
 * borrow-out identity with no conditional branch in the
 * implementation. All arithmetic is unsigned (total, wraps modulo
 * 2^64); the only signed operations are a conversion of the {0,1}
 * flags and a subtraction whose result is confined to {-1, 0, +1}.
 * See cmp.c for the derivation.
 */
int64_t bcmp64(uint64_t a, uint64_t b);

#endif
