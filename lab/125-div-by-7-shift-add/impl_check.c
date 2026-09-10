#include <stdint.h>
#include <stdio.h>

#include "div7.h"

/* Translation unit containing ONLY the shift-add implementation, so the
 * disassembly check inspects the implementation's codegen in isolation
 * (the differential test's / operator must not pollute the check). */

int main(int argc, char **argv)
{
    uint32_t x = (uint32_t)(argc > 1 ? (unsigned long)argv[1][0] : 123456789u);
    uint32_t acc = 0;
    uint32_t i;

    for (i = 0; i < 1000000u; i++) {
        x += 0x9E3779B9u;
        acc += udiv7_shiftadd(x);
    }
    printf("%u\n", acc);
    return 0;
}
