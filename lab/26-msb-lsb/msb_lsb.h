#ifndef MSB_LSB_H
#define MSB_LSB_H

#include <stdint.h>

/*
 * ffs64(x): 1-based index of the lowest set bit of x.
 *   x == 0: returns 0 (same convention as __builtin_ffsll).
 *   x != 0: returns k + 1 where k is the position of the lowest set bit.
 *
 * fls64(x): 0-based index of the highest set bit of x.
 *   x == 0: returns -1 (no set bit exists).
 *   x != 0: returns k where k is the position of the highest set bit.
 */
int ffs64(uint64_t x);
int fls64(uint64_t x);

#endif
