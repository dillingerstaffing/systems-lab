# lab/27-fixed-point

`fixed_point.c` (`fixed_point.h` declares them): Q16.16 fixed-point
add and multiply. A Q16.16 value is an `int32_t` holding
raw / 65536, so the representable range is
[-32768, 32767 + 65535/65536] and one ulp is 1/65536.

- `q16_add(a, b)`: the bit pattern of the sum is the wrapped integer
  sum of the raw patterns, computed in unsigned arithmetic where
  wraparound mod 2^32 is defined. Addition in Q16.16 is exact, so the
  result always equals the true sum, reduced modulo 65536 into the
  representable range. Overflow wraps; there is no saturation.
- `q16_mul(a, b)`: the exact product `(int64_t)a * (int64_t)b` cannot
  overflow, because |a| <= 2^31 and |b| <= 2^31 give |product| <= 2^62
  < 2^63. It is rounded to the nearest Q16.16 value with
  `(p + 32768) >> 16` for non-negative products and the mirrored form
  for negative ones, so exact ties round away from zero. The shifts
  run on non-negative `int64_t` values (always defined), and the
  narrowing to `int32_t` goes through `uint32_t` (defined to wrap mod
  2^32). A rounded product outside the representable range therefore
  wraps instead of saturating.

No signed overflow occurs anywhere, so no undefined behavior is
possible; the ASan+UBSan build confirms this, including on the
wraparound paths.

Verified by `test_fixed_point.c` (fixed-seed splitmix64 PRNG, seed
0x9E3779B97F4A7C15, fully reproducible), differential against two
independent oracles:

- add: exact sum in `int64_t` narrowed with defined wraparound, plus
  an exact double check that the result is congruent to the double sum
  modulo 65536 (all operations exact in double, residual exactly 0).
- mul: a division/remainder-based oracle (different operations from
  the shift-based implementation) computing round-to-nearest, ties
  away from zero, with defined wraparound. In-range cases are also
  checked against the double product and must land within 1 ulp;
  out-of-range cases are checked against the wraparound oracle and the
  double product is confirmed to really be out of range.

Results: 10,001,352 total checks, 0 mismatches, identical checksums
under `-O0`, `-O2`, and ASan+UBSan (see PROOF.md).

- 1,352 directed checks: cross product of 26 values (0, +-1, +-2, +-3,
  0.5, 1.0, 1.5, range edges, `INT32_MIN`, `INT32_MAX`, wraparound
  corners, exact rounding ties such as raw products with fractional
  part exactly 0x8000) for both add and mul.
- 5,000,000 random mul pairs: 70% small-small (products stay in
  range, exercising the double-reference path), 15% mixed, 15%
  full-range (exercising documented wraparound). Of these, 3,677,800
  took the in-range double check, 1,322,876 took the wraparound path,
  and 507 exact rounding ties were observed and confirmed to round
  away from zero.
- 5,000,000 random add pairs over the full 32-bit range.

Timing per op (see PROOF.md for the honest caveats): at `-O2`,
38.78 ns/mul-pair and 29.59 ns/add-pair; at `-O0`, 109.89 ns/mul-pair
and 50.66 ns/add-pair. The timing loops include the PRNG step, the
oracle comparison, the double-reference check, and the checksum
accumulation, so these are the cost of one fully-checked case, not the
cost of one fixed-point op.

Edge-case choices, documented rather than hidden:

- Rounding ties go away from zero (verified by 507 observed ties and
  directed tie cases).
- Overflow wraps modulo 2^32 for both add and mul (matches plain C
  unsigned semantics; no saturation anywhere).
- Representable range is exactly
  [INT32_MIN/65536, INT32_MAX/65536]; anything outside wraps.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), `make run`. See `PROOF.md` for the genuine build log
and run output.
