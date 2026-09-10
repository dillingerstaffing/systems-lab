#include "pdep.h"

/*
 * Loop-scatter identity, one set mask bit at a time:
 *
 *   while mask has set bits:
 *     lowbit = lowest set bit of mask
 *     if lowest src bit is 1, set lowbit in the result
 *     consume that src bit (src >>= 1)
 *     clear lowbit from the mask
 *
 * The lowest remaining src bit is therefore deposited into the lowest
 * remaining set mask position on every iteration, which is exactly the
 * PDEP scatter. No intrinsics, no builtins, no library math; plain C11.
 */
uint64_t pdep64(uint64_t src, uint64_t mask)
{
    uint64_t result = 0;
    uint64_t m = mask;

    while (m != 0) {
        /* Lowest set bit of m, as m & -m in unsigned arithmetic. */
        uint64_t lowbit = m & (~m + 1u);

        if ((src & 1u) != 0)
            result |= lowbit;

        src >>= 1;
        m ^= lowbit;
    }

    return result;
}
