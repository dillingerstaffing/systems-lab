#ifndef HSUM_95_H
#define HSUM_95_H

#include <stdint.h>

/*
 * swar_hsum: horizontal sum of the four packed unsigned 16-bit lanes
 * of a 64-bit word. Lane 0 is bits [15:0], lane 1 is [31:16], lane 2
 * is [47:32], lane 3 is [63:48]. Returns lane0+lane1+lane2+lane3,
 * which is at most 4*65535 = 262140 and always fits in a uint32_t.
 *
 * Defined for every uint64_t input; there is no out-of-contract
 * input. See hsum.c for the derivation and the guard-bit argument
 * that rules out lane crosstalk.
 */
uint32_t swar_hsum(uint64_t w);

#endif
