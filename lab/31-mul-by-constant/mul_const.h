#ifndef MUL_CONST_H
#define MUL_CONST_H

#include <stdint.h>

/*
 * mul_const32: x * k for 8-bit k, computed from the distributive law:
 * x * k = sum over set bits i of k of (x << i). The computation uses
 * only shifts and adds, no multiplication operator. The result wraps
 * modulo 2^32, matching C unsigned arithmetic semantics.
 */
uint32_t mul_const32(uint32_t x, uint8_t k);

#endif
