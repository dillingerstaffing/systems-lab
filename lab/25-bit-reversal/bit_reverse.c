#include "bit_reverse.h"

uint64_t bit_reverse(uint64_t x)
{
    /* Swap adjacent bits: group size 1 -> 2. */
    x = ((x >> 1u) & 0x5555555555555555u) | ((x & 0x5555555555555555u) << 1u);
    /* Swap 2-bit groups: group size 2 -> 4. */
    x = ((x >> 2u) & 0x3333333333333333u) | ((x & 0x3333333333333333u) << 2u);
    /* Swap nibbles within each byte: group size 4 -> 8. */
    x = ((x >> 4u) & 0x0F0F0F0F0F0F0F0Fu) | ((x & 0x0F0F0F0F0F0F0F0Fu) << 4u);
    /* Swap bytes within each 16-bit half: group size 8 -> 16. */
    x = ((x >> 8u) & 0x00FF00FF00FF00FFu) | ((x & 0x00FF00FF00FF00FFu) << 8u);
    /* Swap 16-bit halves within each 32-bit half: 16 -> 32. */
    x = ((x >> 16u) & 0x0000FFFF0000FFFFu) | ((x & 0x0000FFFF0000FFFFu) << 16u);
    /* Swap 32-bit halves: group size 32 -> 64. */
    x = (x >> 32u) | (x << 32u);
    return x;
}
