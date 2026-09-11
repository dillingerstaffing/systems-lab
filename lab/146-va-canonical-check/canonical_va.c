#include "canonical_va.h"

/*
 * The top 26 bits (va >> 38) must be either all zero (a positive
 * 39-bit address, bit 38 clear) or all one (a negative 39-bit address,
 * bit 38 set, sign-extended). One shift, two equality comparisons, no
 * branches, no intrinsics, no builtins.
 */
int canonical_va(uint64_t va)
{
    uint64_t top = va >> 38;
    return (top == 0) || (top == 0x3FFFFFFULL);
}
