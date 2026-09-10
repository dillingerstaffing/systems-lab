#ifndef EQMASK_114_H
#define EQMASK_114_H

#include <stdint.h>

/*
 * swar_eqmask64: bytewise equality mask of two 64-bit words.
 *
 * Returns an 8-bit mask with bit i set iff byte i of x equals byte i
 * of y (byte 0 is the least significant byte). Computed by applying
 * the zero-byte detection identity to d = x ^ y:
 *
 *   t = ((d - 0x0101010101010101ULL) & ~d & 0x8080808080808080ULL)
 *
 * The naive form of that identity is only an existence test: it can
 * flag a nonzero 0x01 byte that receives a borrow from below (e.g.
 * d = 0x0100), so this implementation tests even and odd bytes
 * separately with the other half parked at 0xFF, which can never
 * borrow out, making the per-byte test exact. The eight 0x80 bits
 * are then gathered into the low byte. No conditional branch in the
 * implementation. All arithmetic is unsigned; every shift amount is
 * a compile-time constant in [0, 64). See eqmask.c for the
 * derivation.
 */
uint8_t swar_eqmask64(uint64_t x, uint64_t y);

#endif
