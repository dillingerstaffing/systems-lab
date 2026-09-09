#ifndef CTZ_H
#define CTZ_H

#include <stdint.h>

/*
 * ctz32(x): number of trailing zero bits in x.
 *
 * x != 0: exact count of the zero bits below the lowest set bit.
 * x == 0: defined as 32 (the count of all bits).
 */
unsigned ctz32(uint32_t x);

#endif
