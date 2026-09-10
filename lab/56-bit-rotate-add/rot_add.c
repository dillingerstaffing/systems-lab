#include "rot_add.h"

uint64_t rotl64(uint64_t x, unsigned k)
{
    k %= 64u;
    /* k == 0 must return early: shifting a 64-bit value by 64 is
       undefined behavior in C, and (64u - 0) would be exactly that. */
    if (k == 0u)
        return x;
    return (x << k) | (x >> (64u - k));
}

uint64_t rot_add64(uint64_t x, uint64_t y, unsigned k)
{
    /* Unsigned addition is defined to wrap modulo 2^64, so this is
       the sum of the rotated value and y, no overflow check needed. */
    return rotl64(x, k) + y;
}
