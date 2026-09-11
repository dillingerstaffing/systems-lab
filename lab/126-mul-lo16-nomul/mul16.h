#ifndef MUL16_H
#define MUL16_H

#include <stdint.h>

/*
 * Low 16 bits of the 32-bit product a*b, computed with shifts and adds
 * only. No multiply operator, no intrinsics or builtins anywhere in the
 * implementation.
 */
uint16_t mullo16(uint16_t a, uint16_t b);

#endif /* MUL16_H */
