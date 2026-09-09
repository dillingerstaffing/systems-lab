#ifndef HAMMING_H
#define HAMMING_H

#include <stdint.h>

/* hamming64: number of bit positions where a and b differ (0..64). */
uint32_t hamming64(uint64_t a, uint64_t b);

#endif
