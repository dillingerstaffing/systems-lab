#include "select.h"

/*
 * bselect(sel, a, b) from the mask identity, no comparison and no
 * branch.
 *
 * Fact 1: unsigned negation wraps modulo 2^64 (C11 6.2.5p9), so it is
 * total: for every uint64_t x, -x = 2^64 - x exactly, with -0 = 0.
 * Therefore for sel in {0,1}:
 *   sel = 0 -> mask = (uint64_t)(-(uint64_t)0) = 0
 *   sel = 1 -> mask = (uint64_t)(-(uint64_t)1) = 2^64 - 1 (all bits 1)
 * No signed arithmetic is performed, so no overflow or undefined
 * behavior is possible.
 *
 * Fact 2: AND/OR/NOT are bitwise, so for the two contract values:
 *   sel = 0: out = (a & ~0) | (b & 0) = (a & all1) | 0 = a
 *   sel = 1: out = (a & ~all1) | (b & all1) = 0 | b = b
 * The two mask values are complements, so the halves select a and b
 * disjointly and cover every bit exactly once.
 *
 * Contract: sel in {0,1}. Outside the contract the mask is some other
 * value (e.g. sel = 2 gives mask = 2^64 - 2 = ...1110, sel =
 * UINT64_MAX gives mask = 1), and out is the corresponding arithmetic
 * blend, not a defined selection. The contract rows in test_select.c
 * print such cases and never differential-check them.
 */
uint64_t bselect(uint64_t sel, uint64_t a, uint64_t b)
{
    uint64_t mask = (uint64_t)(-(uint64_t)sel);
    return (a & ~mask) | (b & mask);
}
