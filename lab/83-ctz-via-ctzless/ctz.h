#ifndef CTZ_H
#define CTZ_H

#include <stdint.h>

/*
 * Trailing-zero count of a nonzero 64-bit word, built only from the
 * identity
 *
 *     ctz(x) = popcount((x ^ (x-1)) >> 1)
 *
 * where the popcount is a hand-rolled SWAR bit count (ctz.c), not a
 * compiler intrinsic. No __builtin_ctz, no __builtin_popcountll, no
 * lookup tables, only <stdint.h> types.
 *
 * Why the identity holds: for nonzero x, subtracting 1 borrows through
 * exactly the trailing zeros, so x-1 flips the lowest set bit and every
 * bit below it. x ^ (x-1) is therefore a run of (ctz(x)+1) ones in the
 * low bits; shifting right by 1 drops one of them and leaves exactly
 * ctz(x) ones, which popcount counts.
 *
 * Contract: x must be nonzero. Passing x == 0 is out of contract; the
 * function is observed to return 63 in that case, which carries no
 * meaning and must not be relied upon (pinned by a test row, but not
 * part of the contract).
 */
uint32_t ctz64_identity(uint64_t x);

#endif
