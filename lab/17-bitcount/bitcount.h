#ifndef BITCOUNT_H
#define BITCOUNT_H

#include <stdint.h>

/* Number of 1 bits in x, using the SWAR parallel-addition identities.
 * No libc, only <stdint.h>. */
uint32_t popcount64(uint64_t x);

#endif
