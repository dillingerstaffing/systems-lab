#ifndef ROUNDUP_H
#define ROUNDUP_H

#include <stdint.h>

/*
 * round_up_pow2_64: round x up to the next power of two.
 *
 * Contract (all in unsigned arithmetic, which wraps):
 *   - x == 0 maps to 0. (x - 1) wraps to all ones, the cascade keeps
 *     all ones, and adding 1 wraps back to 0.
 *   - x already a power of two is identity: (x - 1) has all lower
 *     bits set, the cascade changes nothing, +1 restores x.
 *   - x above 2^63 maps to 0: the smallest representable power of
 *     two would be 2^64, which is unrepresentable in 64 bits; the
 *     cascade produces all ones and +1 wraps to 0. This limit is
 *     pinned by dedicated test rows.
 *
 * Implemented with the shift/OR fill cascade on (x - 1), then +1.
 * No builtins, no intrinsics.
 */
uint64_t round_up_pow2_64(uint64_t x);

#endif
