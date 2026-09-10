#ifndef ZERO_RUN_H
#define ZERO_RUN_H

#include <stdint.h>

/*
 * longest_zero_run(x): length of the longest run of consecutive zero
 * bits in the 64-bit word x.
 *
 * x == 0: defined as 64 (the whole word is one zero-run).
 * x == 0xFFFFFFFFFFFFFFFF: 0 (there is no zero bit at all).
 *
 * How it works: y = ~x turns every zero-run of x into a one-run of y.
 * One round of y &= y << 1 keeps bit i exactly when bits i and i-1
 * were both set the round before, so each round deletes the lowest
 * set bit of every maximal one-run. After k rounds, bit i is set
 * exactly when bits i-k..i were all set in the original y, so y is
 * nonzero after k rounds exactly when the original y held a one-run
 * of length k+1 or more. The number of rounds until y reaches 0 is
 * therefore the length of the longest one-run of y, which is the
 * length of the longest zero-run of x. Only shifts and AND are used;
 * no count builtins and no bit-scan instructions.
 */
unsigned longest_zero_run(uint64_t x);

#endif
