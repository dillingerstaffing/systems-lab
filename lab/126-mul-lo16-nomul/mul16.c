#include <stdint.h>

#include "mul16.h"

uint16_t mullo16(uint16_t a, uint16_t b)
{
    /*
     * The full product is the sum over all pairs of bit positions of
     * a_i * b_j * 2^(i+j). Truncating the product to 16 bits discards
     * every term with i+j >= 16, so the low 16 bits depend only on the
     * low 16 bits of each operand.
     *
     * Shift-add over the set bits of b: for each set bit j of b, the
     * term is a * 2^j, which is exactly a << j. Accumulate the partial
     * sums in a 32-bit register and truncate to 16 bits at the end; the
     * truncation is what the mathematical identity above requires.
     *
     * Nothing here but shifts, adds, and one loop: no multiply operator,
     * no intrinsics, no builtins.
     */
    uint32_t acc = 0;
    for (unsigned j = 0; j < 16; j++) {
        if (b & 1u)
            acc += (uint32_t)a << j;
        b >>= 1;
    }
    return (uint16_t)acc;
}
