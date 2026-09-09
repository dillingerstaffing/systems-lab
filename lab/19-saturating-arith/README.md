# lab/19-saturating-arith

`saturating.c` (`saturating.h` declares them): 32-bit saturating add and
sub, clamped to `INT32_MIN`/`INT32_MAX` on overflow instead of wrapping.

- `sat_add32(a, b)`: the sum is formed in unsigned arithmetic (where
  wraparound is defined); overflow is detected purely from the three
  sign bits. In two's complement, `a + b` overflows exactly when both
  addends share a sign and the wrapped sum carries the opposite sign.
  With mixed signs the true sum lies between the two operands, so
  overflow is impossible.
- `sat_sub32(a, b)`: same construction on `a - b`. Overflow exactly
  when the subtrahend's sign differs from the minuend's and the wrapped
  difference carries a sign different from the minuend's. With matching
  signs the true difference lies between the operands, so overflow is
  impossible.

No signed overflow ever occurs, so no undefined behavior is possible;
the conversion back to `int32_t` runs only when the wrapped value
equals the true sum/difference and is in range. Verified by
`test_saturating.c` (fixed-seed xorshift64* PRNG, fully reproducible),
differential against the trivially correct oracle: the exact result in
`int64_t` (which cannot overflow for 32-bit operands), clamped to
`[INT32_MIN, INT32_MAX]`:

- 10,000,338 total checks, 0 mismatches, identical checksums under
  `-O0`, `-O2`, and ASan+UBSan (see PROOF.md).
- 338 directed checks: cross product of `INT32_MIN`, `INT32_MAX`, 0,
  +-1, +-2, +-3 and the values adjacent to the saturation points, for
  both add and sub (every corner case: `INT32_MAX + 1`,
  `INT32_MIN - 1`, `0 - INT32_MIN`, etc.).
- 5,000,000 fixed-seed random pairs per op (10,000,000 total).

Timing per op (see PROOF.md for the honest caveats): at `-O2`,
18.19 ns/add and 17.70 ns/sub; at `-O0`, 77.44 ns/add and
38.51 ns/sub (the add sweep runs first on a cold cache, hence the
gap).

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), `make run`. See `PROOF.md` for the genuine build log
and run output.
