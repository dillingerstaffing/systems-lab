#include "mul_const.h"

uint32_t mul_const32(uint32_t x, uint8_t k)
{
    /*
     * k = b0 + 2*b1 + 4*b2 + ... + 128*b7 with each bi in {0,1}.
     * Distributing x over that sum gives x * b_i * 2^i for each set
     * bit i, and x * 2^i is exactly x << i. All arithmetic is
     * unsigned, so every shift and add is defined and the running
     * total wraps modulo 2^32, exactly what C specifies for the
     * product x * k in 32 bits.
     */
    uint32_t acc = 0u;
    uint32_t bit = x;

    for (unsigned i = 0; i < 8; ++i) {
        if (((k >> i) & 1u) != 0u)
            acc += bit;
        bit <<= 1;
    }

    return acc;
}
