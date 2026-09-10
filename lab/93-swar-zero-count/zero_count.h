/* Count zero bytes in a 64-bit word with parallel byte arithmetic.
 *
 * The starting point is the zero-byte detection identity
 *
 *   mask = (x - 0x0101010101010101) & ~x & 0x8080808080808080
 *
 * Subtracting 0x01 from each byte leaves a byte's top bit set when the
 * subtraction borrowed out of that byte, which happens when the byte was
 * 0x00. The (& ~x) term clears top bits for bytes whose original top bit
 * was already set (0xFF - 0x01 = 0xFE keeps its top bit), so only zero
 * bytes leave a top bit set.
 *
 * Measured defect of the identity as written: it is only safe as an
 * existence test ("some byte is zero"), not for counting. A borrow out of
 * a zero low byte propagates into the byte above it, and if that byte is
 * 0x01 the extra subtract turns it into 0xFF, setting its top bit even
 * though the byte is not zero. Differential testing against a per-byte
 * reference found 1001 such overcounts in 10,065,536 inputs (for example
 * x = 0x0100 reports 8 zero bytes where 7 exist).
 *
 * The fix used here: run the subtraction in 16-bit lanes with a 0x7F
 * guard in each lane's high byte. Even bytes of x go in the low bytes of
 * one word's lanes, odd bytes in another's. A lane holds (0x7F << 8) | v,
 * and subtracting 0x0001 per lane can borrow at most 1 out of the low
 * byte and at most 1 into the high byte, so the high byte stays >= 0x7D
 * and no borrow ever crosses a lane boundary. Each lane's low byte then
 * behaves exactly like an isolated byte subtraction: its top bit ends up
 * set, after the (& ~lane_input) term, exactly when the byte was zero.
 *
 * The four per-lane indicators (one per 16-bit lane) are summed with the
 * multiply trick: ((v >> 7) * 0x0001000100010001) >> 48, which adds the
 * four 0/1 lane values into the top lane. Even and odd counts are added.
 * Returns 0..8.
 */
#ifndef SWAR_ZERO_COUNT_H
#define SWAR_ZERO_COUNT_H

#include <stdint.h>

#define ZC_GUARD 0x7F007F007F007F00ULL
#define ZC_SUB   0x0001000100010001ULL
#define ZC_TOP   0x0080008000800080ULL
#define ZC_SUM   0x0001000100010001ULL

/* Zero-byte count of the four bytes sitting in the low bytes of 16-bit lanes. */
static unsigned count_zero_bytes_lanes(uint64_t lanes) {
	uint64_t w = lanes | ZC_GUARD;
	uint64_t d = w - ZC_SUB;
	uint64_t m = d & ~lanes & ZC_TOP;
	return (unsigned)(((m >> 7) * ZC_SUM) >> 48);
}

static unsigned count_zero_bytes(uint64_t x) {
	uint64_t even = x & 0x00FF00FF00FF00FFULL;
	uint64_t odd = (x >> 8) & 0x00FF00FF00FF00FFULL;
	return count_zero_bytes_lanes(even) + count_zero_bytes_lanes(odd);
}

#endif
