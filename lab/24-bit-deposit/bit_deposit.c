#include "bit_deposit.h"

/*
 * mask_n: the low n bits set. mask_n = (1ULL << n) - 1 for n < 64;
 * n = 64 is the special case (1ULL << 64 is undefined), where the
 * mask is all 64 bits set.
 */
static uint64_t mask_n(unsigned n)
{
    return n == 64u ? UINT64_MAX : ((1ULL << n) - 1u);
}

uint64_t bit_extract(uint64_t x, unsigned off, unsigned w)
{
    return (x >> off) & mask_n(w);
}

uint64_t bit_insert(uint64_t x, unsigned off, unsigned w, uint64_t v)
{
    uint64_t m = mask_n(w);
    return (x & ~(m << off)) | ((v & m) << off);
}
