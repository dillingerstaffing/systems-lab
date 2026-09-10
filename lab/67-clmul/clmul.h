/*
 * clmul.h - carryless 64x64 -> 128-bit multiply over GF(2).
 *
 * Treats each operand as a polynomial over GF(2) (bit i is the
 * coefficient of x^i) and returns their polynomial product.  Addition
 * of coefficients is XOR, so no carries propagate: result bit j is
 * the XOR, over all i, of bit_i(a) AND bit_{j-i}(b).
 *
 * Computed with the shift-xor identity:
 *     clmul(a, b) = XOR over set bits i of a of (b << i)
 * No lookup tables, no intrinsics, no library calls.
 */
#ifndef CLMUL_H
#define CLMUL_H

#include <stdint.h>

typedef struct {
    uint64_t hi;
    uint64_t lo;
} clmul128;

static inline clmul128 clmul64(uint64_t a, uint64_t b)
{
    uint64_t lo = 0, hi = 0;
    for (int i = 0; i < 64; i++) {
        uint64_t m = 0ULL - ((a >> i) & 1ULL);
        lo ^= (b << i) & m;
        hi ^= (i == 0) ? 0ULL : (b >> (64 - i)) & m;
    }
    return (clmul128){ .hi = hi, .lo = lo };
}

#endif /* CLMUL_H */
