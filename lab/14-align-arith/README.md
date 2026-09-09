# lab/14-align-arith

Alignment and power-of-two rounding for `uint32_t`, implemented with
bit identities in `align.c` (`align.h` declares the four functions).
No libc in the implementation, only `<stdint.h>`.

What it does:

- `align_down_u32(p, a)` = `p & ~(a - 1)`: round p down to a multiple of a.
- `align_up_u32(p, a)` = `(p + (a - 1)) & ~(a - 1)`: round p up to a multiple of a.
- `is_pow2_u32(x)`: `x != 0 && (x & (x - 1)) == 0`.
- `round_up_pow2_u32(x)`: smallest power of two >= x, via the shift/or
  propagation (`x |= x >> 1; x |= x >> 2; x |= x >> 4; x |= x >> 8;
  x |= x >> 16`) identity. Returns 1 for x <= 1. Overflow is defined:
  returns 0 when the answer does not fit in 32 bits (x > 0x80000000),
  including x = UINT32_MAX.

Precondition: `a` is a power of two with a >= 1. For `align_up`,
p + (a - 1) can wrap modulo 2^32 when p is near UINT32_MAX; unsigned
wrap is well-defined C, and the tests verify the wrapped result
exactly against the division reference under the same wrap.

Verified by `test_align.c` (fixed-seed PRNG, differential against
division/loop-based references):

- 4,200,728 total checks, 0 mismatches, identical output under
  `-O0`, `-O2`, and ASan+UBSan (see PROOF.md).
- Boundary sweep: every 2^k and its neighbors (2^k - 1, 2^k + 1) for
  k = 0..31, plus UINT32_MAX, crossed with every power-of-two
  alignment 1..2^31 for align_up/align_down.
- 1,048,576 random 32-bit values per primitive.
- 22 edge cases asserted directly: x = 0, x = UINT32_MAX, a = 1,
  and round_up_pow2 overflow returning 0.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), `make run`. See `PROOF.md` for the genuine build log
and run output.
