/* sat_sub.h - unsigned saturating subtraction for 64-bit operands. */
#ifndef SAT_SUB_H
#define SAT_SUB_H

#include <stdint.h>

/* sat_sub64(a, b) = a - b when a >= b, else 0. The borrow-out of the
 * unsigned subtraction a - b is exactly the comparison a < b (underflow
 * happens iff b > a), so saturation reduces to masking the difference
 * to zero on borrow. Branchless; uses only well-defined unsigned
 * arithmetic; no __int128 in the implementation. */
uint64_t sat_sub64(uint64_t a, uint64_t b);

#endif /* SAT_SUB_H */
