#include "eqmask.h"

/*
 * swar_eqmask64(x, y): bit i of the result is 1 iff byte i of x
 * equals byte i of y. Byte i means bits [8i, 8i+8) of the word.
 *
 * Work entirely in the unsigned domain (C11 6.2.5p9: unsigned
 * arithmetic wraps modulo 2^64, so it is total; every shift is by a
 * constant in [0, 64), so no shift is undefined).
 *
 * Step 1: d = x ^ y. Byte i of d is x_i ^ y_i, which is 0 iff
 * x_i == y_i (XOR is bitwise: it is zero in a byte exactly when the
 * two bytes agree in every bit). So the problem is an exact
 * zero-byte mask of d.
 *
 * Step 2: the zero-byte identity, and why its naive form is not an
 * exact mask. For byte i of d write d_i, and let c_i in {0, 1} be
 * the borrow into byte i in (d - 0x0101010101010101); byte i of the
 * difference is (d_i - 1 - c_i) mod 256. The candidate
 *   t = ((d - 0x0101010101010101) & ~d & 0x8080808080808080)
 * sets bit 8i+7 when that byte's bit 7 and (~d)'s bit 7 are both 1.
 * If d_i = 0 the byte is 0xFF (c_i = 0) or 0xFE (c_i = 1), bit 7
 * set, and ~d has 0xFF, bit 7 set: genuine zero bytes are always
 * flagged. But with d_i != 0 both bit 7s can still be 1: with
 * c_i = 0, (d_i - 1) has bit 7 set only for d_i >= 129 while ~d_i
 * has bit 7 set only for d_i <= 127 (disjoint, no false positive);
 * with c_i = 1, (d_i - 2) mod 256 has bit 7 set for d_i = 1 (0xFF)
 * or d_i >= 130, and ~d_i has bit 7 set for d_i <= 127, whose only
 * common value is d_i = 1. So the naive form flags a nonzero byte
 * 0x01 whenever a borrow arrives from below (e.g. d = 0x0100: byte
 * 0 borrows, byte 1 = 0x01 gets flagged). It is an existence test,
 * not an exact mask; the differential test caught exactly this.
 *
 * Step 3 (borrow suppression): test even and odd bytes separately,
 * parking the other half at 0xFF, which can never borrow out
 * (0xFF - 1 - c >= 0xFD for c in {0, 1}). With
 *   de = (d & 0x00FF00FF00FF00FF) | 0xFF00FF00FF00FF00,
 * every even byte of (de - 0x0101010101010101) sees borrow-in 0:
 * byte 0 by definition, and byte 2i (i > 0) because odd byte 2i-1
 * is 0xFF and emits no borrow. So the step-2 per-byte analysis with
 * c_i = 0 applies exactly: bit 8i+7 of
 *   ((de - 0x0101010101010101) & ~de & 0x8080808080808080)
 * is 1 iff byte 2i of d is 0, and its odd bytes are 0x00 (~de is
 * 0x00 there). Symmetrically,
 *   do = (d & 0xFF00FF00FF00FF00) | 0x00FF00FF00FF00FF
 * yields the exact odd-byte mask with 0x00 even bytes. ORing the
 * two gives t with 0x80 in byte i iff d_i = 0, exactly, for all
 * eight bytes, with no false positives and no false negatives.
 *
 * Step 4 (gather): u = t >> 7 has bit 8i set exactly for the equal
 * bytes. Term ((u >> 7i) & (1 << i)) reads bit 8i of u (bit i of
 * u >> 7i is bit i + 7i = 8i) and places it at output bit i. ORing
 * the eight disjoint terms yields a byte whose bit i is 1 iff byte
 * i matched. The terms are disjoint single bits, so no carry or
 * overlap is possible.
 *
 * The -O2 disassembly excerpt in PROOF.md shows straight-line code
 * with no conditional jump.
 */
uint8_t swar_eqmask64(uint64_t x, uint64_t y)
{
    uint64_t d = x ^ y;
    uint64_t de = (d & 0x00FF00FF00FF00FFULL) | 0xFF00FF00FF00FF00ULL;
    uint64_t te = ((de - 0x0101010101010101ULL) & ~de) & 0x8080808080808080ULL;
    uint64_t do_ = (d & 0xFF00FF00FF00FF00ULL) | 0x00FF00FF00FF00FFULL;
    uint64_t to = ((do_ - 0x0101010101010101ULL) & ~do_) & 0x8080808080808080ULL;
    uint64_t t = te | to;
    uint64_t u = t >> 7;
    return (uint8_t)(((u >>  0) & 0x01u) | ((u >>  7) & 0x02u) |
                     ((u >> 14) & 0x04u) | ((u >> 21) & 0x08u) |
                     ((u >> 28) & 0x10u) | ((u >> 35) & 0x20u) |
                     ((u >> 42) & 0x40u) | ((u >> 49) & 0x80u));
}
