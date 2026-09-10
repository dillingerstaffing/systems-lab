#ifndef ISO_H
#define ISO_H

#include <stdint.h>

/* iso_lowest(x): isolate the lowest set bit of x via the identity
 * iso(x) = x & -x, where -x is computed in unsigned arithmetic as
 * (0 - x) mod 2^64, so no signed operation is involved.
 * Returns 0 when x is 0. */
uint64_t iso_lowest(uint64_t x);

#endif
