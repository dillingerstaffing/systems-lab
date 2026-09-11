#include "cond_neg.h"

/*
 * The two's-complement conditional negation:
 *
 *   f = 0: -f = 0, so (x ^ 0) + 0 = x
 *   f = 1: -f = all ones, so (x ^ all-ones) + 1 = ~x + 1 = -x
 *
 * All arithmetic is unsigned, so -f, ^, and + are all defined for every
 * input; the only caller contract is f in {0, 1}. Compiles to neg/xor/add
 * with no branch.
 */
uint64_t cond_neg(uint64_t x, uint64_t f)
{
    return (x ^ -f) + f;
}
