#ifndef F32_H
#define F32_H

#include <stdint.h>

/*
 * f32_add / f32_mul take and return IEEE-754 binary32 values as raw
 * uint32_t bit patterns. The implementations use integer bit manipulation
 * only; no float operations appear in this translation unit.
 */
uint32_t f32_add(uint32_t a, uint32_t b);
uint32_t f32_mul(uint32_t a, uint32_t b);

#endif
