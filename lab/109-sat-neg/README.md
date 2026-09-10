# lab/109-sat-neg

Saturating negation of an `int64_t`, `neg_sat64(x)`: returns
`INT64_MAX` when `x == INT64_MIN`, and `-x` otherwise.

The construction, from the sign-mask identity with the
`INT64_MIN` edge folded into the mask, in unsigned (mod 2^64)
arithmetic with `u = (uint64_t)x` and `s = (uint64_t)(x >> 63)`:

- Base: `s - u + (s & 1)`. For `x >= 0` this is `0 - u + 0`,
  the two's complement negation. For `x < 0`, `x != INT64_MIN`,
  it is `(2^64 - 1) - u + 1 = 2^64 - u`, again the exact
  negation. For `x == INT64_MIN` it wraps to `2^63`, one more
  than the correct answer.
- Edge detector, no comparison: `d = u ^ 0x8000000000000000`
  is zero exactly for `x == INT64_MIN`, and for `d != 0` at
  least one of `d`, `-d` (mod 2^64) has bit 63 set, so
  `e = 1 - ((d | (0 - d)) >> 63)` is 1 exactly on the edge.
- Final: `neg_sat64(x) = s - u + ((s & 1) ^ e)`, which subtracts
  the extra 1 on the edge row and is identical elsewhere.

No branch, no comparison anywhere; the test disassembles the
`-O2` object file with `objdump` and fails on any conditional
jump. No signed overflow anywhere: every operation is on
`uint64_t`.

Verified in `test_sat_neg.c`:

- Dedicated `INT64_MIN` row: `neg_sat64(INT64_MIN)` is
  `INT64_MAX`.
- All 65,536 signed 16-bit inputs, exhaustive, differential-checked
  against the ternary reference `x == INT64_MIN ? INT64_MAX : -x`.
- 10,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked against the
  reference.
- 10,065,537 total checks, 0 mismatches. FNV-1a 64-bit checksum
  over the result stream: `e52a81b464c7559c`, identical across
  `-O0`, `-O2`, and ASan+UBSan builds.
- The `-O2` disassembly of `neg_sat64` contains no conditional
  jump (checked programmatically in the test and independently by
  grep).
- Measured at -O2: `6.21` ns/value (best of 5 reps over a 1M-value
  buffer).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory for the correctness builds, `make
bench` for throughput. The genuine build log and run output are in
`PROOF.md`.
