#include "decimal_digits.h"

uint8_t digit_count(uint64_t x)
{
    /* 10^k for k = 1..19 as unsigned 64-bit literals. 10^19 is
     * 10000000000000000000, below 2^64 - 1, so every literal is exact
     * and in range. Each (x >= 10^k) is 1 exactly when x has more than
     * k digits, so the sum counts how many powers of ten x reaches. */
    return (uint8_t)(1
        + (x >= 10ULL)
        + (x >= 100ULL)
        + (x >= 1000ULL)
        + (x >= 10000ULL)
        + (x >= 100000ULL)
        + (x >= 1000000ULL)
        + (x >= 10000000ULL)
        + (x >= 100000000ULL)
        + (x >= 1000000000ULL)
        + (x >= 10000000000ULL)
        + (x >= 100000000000ULL)
        + (x >= 1000000000000ULL)
        + (x >= 10000000000000ULL)
        + (x >= 100000000000000ULL)
        + (x >= 1000000000000000ULL)
        + (x >= 10000000000000000ULL)
        + (x >= 100000000000000000ULL)
        + (x >= 1000000000000000000ULL)
        + (x >= 10000000000000000000ULL));
}
