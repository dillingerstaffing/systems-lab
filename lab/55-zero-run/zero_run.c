#include "zero_run.h"

unsigned longest_zero_run(uint64_t x)
{
    uint64_t y = ~x;
    unsigned n = 0u;

    while (y != 0u) {
        y &= y << 1;
        ++n;
    }
    return n;
}
