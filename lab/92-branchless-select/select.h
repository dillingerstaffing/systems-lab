#ifndef SELECT_92_H
#define SELECT_92_H

#include <stdint.h>

/*
 * bselect: branchless 2-way select. Returns b if sel == 1, a if
 * sel == 0.
 *
 * Contract: sel must be 0 or 1. For any other sel the result is the
 * raw arithmetic value of the mask identity below, not a defined
 * selection; callers must never rely on it. See select.c for the
 * derivation and the out-of-contract rows in test_select.c.
 */
uint64_t bselect(uint64_t sel, uint64_t a, uint64_t b);

#endif
