#ifndef INC128_CARRY_H
#define INC128_CARRY_H

#include <stdint.h>

/* A 128-bit value as two 64-bit words, low word first. */
typedef struct {
    uint64_t hi;
    uint64_t lo;
} u128;

/* Result of inc128: the incremented value, plus the carry out of
 * bit 127 (1 exactly when the input was all ones). */
typedef struct {
    uint64_t hi;
    uint64_t lo;
    int carry;
} inc128_res;

/*
 * inc128: add 1 to a 128-bit value with carry cascading from the
 * low word to the high word.
 *
 * Construction, from unsigned wraparound (C11 6.2.5p9: unsigned
 * arithmetic is modulo 2^N):
 *
 * 1. lo2 = x.lo + 1 is (x.lo + 1) mod 2^64. lo2 == 0 holds exactly
 *    when x.lo + 1 == 2^64, i.e. x.lo == UINT64_MAX. That is the
 *    carry into the high word: carry = (lo2 == 0).
 *
 * 2. hi2 = x.hi + carry. With carry in {0,1}, hi2 == 0 holds exactly
 *    when carry == 1 and x.hi == UINT64_MAX.
 *
 * 3. The carry out of bit 127 is 1 exactly when the whole 128-bit
 *    input was all ones (x.hi == UINT64_MAX and x.lo == UINT64_MAX).
 *    By (1) and (2), (lo2 == 0 && hi2 == 0) holds exactly in that
 *    case, and only then.
 *
 * No 128-bit type, no intrinsics, no builtins are used here.
 */
inc128_res inc128(u128 x);

#endif
