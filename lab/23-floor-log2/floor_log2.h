#ifndef FLOOR_LOG2_H
#define FLOOR_LOG2_H

#include <stdint.h>

/*
 * floor_log2(x): index of the highest set bit of x, i.e. the unique k
 * with 2^k <= x < 2^(k+1) for x != 0.
 *
 * Domain convention: x = 0 is excluded from the domain because log2(0)
 * is undefined. The implementation still returns a defined value for
 * 0: the propagation leaves 0, popcount(0) is 0, and 0 - 1 wraps to
 * UINT64_MAX in unsigned arithmetic. Callers must not rely on the 0
 * case; the test suite checks the convention but runs the reference
 * comparison only for nonzero inputs.
 *
 * Method: shift/OR propagation (x |= x >> 1; x |= x >> 2; ... >> 32)
 * fills every bit at or below the highest set bit, so the result has
 * exactly k + 1 bits set; subtracting 1 gives k. No clz instruction
 * or compiler builtin is used in the implementation.
 */
uint64_t floor_log2(uint64_t x);

#endif
