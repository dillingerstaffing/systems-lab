#ifndef SIGN_87_H
#define SIGN_87_H

#include <stdint.h>

/*
 * sign64: returns -1 if x < 0, 0 if x == 0, +1 if x > 0.
 *
 * Built from two's-complement bit facts only: no comparison, no
 * branch, no division, no library call. See sign.c for the
 * derivation, including the INT64_MIN contract.
 */
int64_t sign64(int64_t x);

#endif
