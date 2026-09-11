#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitop.h"

/* Translation unit containing ONLY the single-bit operations, so the
 * disassembly check inspects the implementation's codegen in isolation.
 * Positions come from argv at runtime, so the assert(i < 64) guards and
 * the variable-shift codegen cannot be optimized away. */

int main(int argc, char **argv)
{
    uint64_t x = 0x0123456789ABCDEFull;
    int i = argc > 1 ? atoi(argv[1]) : 63;
    uint64_t acc = 0;
    uint64_t n;

    if (i < 0 || i > 63)
        i = 63;
    for (n = 0; n < 1000000u; n++) {
        x += 0x9E3779B97F4A7C15ull;
        i = (i * 37 + 11) % 64; /* stays in 0..63, opaque to the compiler */
        acc += bset(x, i) + bclr(x, i) + btg(x, i) + (uint64_t)btst(x, i);
    }
    printf("%llu\n", (unsigned long long)acc);
    return 0;
}
