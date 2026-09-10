#ifndef ROT_ADD_H
#define ROT_ADD_H

#include <stdint.h>

/* Rotate x left by k bit positions (k taken modulo 64). */
uint64_t rotl64(uint64_t x, unsigned k);

/* Rotate x left by k, then add y with unsigned wraparound. */
uint64_t rot_add64(uint64_t x, uint64_t y, unsigned k);

#endif
