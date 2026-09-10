#include "iso.h"

uint64_t iso_lowest(uint64_t x)
{
    /* -x for unsigned x is defined as (0 - x) mod 2^64, so the
     * identity iso(x) = x & -x is evaluated with unsigned
     * arithmetic only. x & (0 - x) keeps exactly the bit at the
     * position of the lowest set bit of x and clears everything
     * else. */
    return x & ((uint64_t)0 - x);
}
