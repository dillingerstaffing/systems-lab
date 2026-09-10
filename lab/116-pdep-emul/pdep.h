#ifndef PDEP_H
#define PDEP_H

#include <stdint.h>

/*
 * pdep64: software parallel-bit-deposit for 64-bit words.
 *
 * Scatter the low popcount(mask) bits of src into the set bit positions
 * of mask. The lowest remaining src bit lands in the lowest set mask
 * position, the next in the next, and so on; src bits beyond the low
 * popcount(mask) are ignored. Equivalent to the PDEP instruction of
 * x86 BMI2, computed with plain shifts, masks, and a loop.
 *
 * The implementation follows the loop-scatter identity: walk the mask
 * bits from low to high; for each set mask bit, take the current lowest
 * src bit, deposit it at that mask position, then shift src right by one.
 */
uint64_t pdep64(uint64_t src, uint64_t mask);

#endif /* PDEP_H */
