#ifndef COND_NEG_H
#define COND_NEG_H

#include <stdint.h>

/*
 * cond_neg: branchless conditional negation for 64-bit words.
 *
 * cond_neg(x, f) = (x ^ -f) + f, where f is in {0, 1}.
 *
 * Why this works, from the two's-complement negation identity:
 * -f in unsigned 64-bit arithmetic is 0 when f is 0 and 2^64 - 1
 * (all ones) when f is 1. XOR with all ones flips every bit, which is
 * bitwise NOT; adding 1 after bitwise NOT is exactly two's-complement
 * negation (~x + 1 = -x mod 2^64). So for f = 1 the expression is
 * ~x + 1 = -x, and for f = 0 it is x ^ 0 + 0 = x. No comparison and no
 * branch on the flag appears anywhere in the computation.
 */
uint64_t cond_neg(uint64_t x, uint64_t f);

#endif /* COND_NEG_H */
