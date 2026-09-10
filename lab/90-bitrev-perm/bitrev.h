#ifndef BITREV_H
#define BITREV_H

#include <stdint.h>

/* out[bitrev(i)] = in[i] for i = 0..7, where bitrev is the 3-bit
 * index reversal of an 8-element array. */
void bitrev8_perm(const uint32_t in[8], uint32_t out[8]);

#endif
