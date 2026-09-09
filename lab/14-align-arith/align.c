#include "align.h"

uint32_t align_down_u32(uint32_t p, uint32_t a)
{
    /* For power-of-two a, a - 1 is the low-bit mask; clearing those bits
     * rounds down to a multiple of a. */
    return p & ~(a - 1u);
}

uint32_t align_up_u32(uint32_t p, uint32_t a)
{
    /* Adding a - 1 pushes any non-multiple past the next boundary, and
     * the mask then clears the low bits. Unsigned wrap on p + (a - 1)
     * is defined; see the header for the stated wrap behavior. */
    return (p + (a - 1u)) & ~(a - 1u);
}

int is_pow2_u32(uint32_t x)
{
    /* A power of two has exactly one bit set, so subtracting one flips
     * that bit to zero and sets all lower bits; the AND is then zero.
     * x = 0 must be excluded explicitly (0 & (0 - 1) == 0 is a false hit). */
    return x != 0u && (x & (x - 1u)) == 0u;
}

uint32_t round_up_pow2_u32(uint32_t x)
{
    if (x <= 1u) {
        return 1u;
    }
    if (x > UINT32_C(0x80000000)) {
        /* The answer would be 2^32, which does not fit: defined as 0. */
        return 0u;
    }
    /*
     * Shift/or propagation: after these steps every bit at or below the
     * highest set bit is 1 (for x - 1), so adding 1 carries to the next
     * power of two. 1, 2, 4, 8, 16 cover all 32 bits.
     */
    x -= 1u;
    x |= x >> 1u;
    x |= x >> 2u;
    x |= x >> 4u;
    x |= x >> 8u;
    x |= x >> 16u;
    return x + 1u;
}
