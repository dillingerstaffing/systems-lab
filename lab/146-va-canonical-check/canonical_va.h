#ifndef CANONICAL_VA_H
#define CANONICAL_VA_H

#include <stdint.h>

/*
 * Sv39 canonical virtual address test.
 *
 * In Sv39, the usable virtual address is 39 bits wide (bit 38 down to
 * bit 0). The address is canonical when bits 63:39 are all equal to bit
 * 38, the sign extension of the 39-bit address into the full 64-bit
 * register. Any other pattern in the top 26 bits is non-canonical and
 * raises a page fault on use.
 *
 * Returns nonzero if va is canonical, zero otherwise.
 */
int canonical_va(uint64_t va);

#endif
