# lab/96-isolate-lowest

`iso_lowest(x)` isolates the lowest set bit of a `uint64_t` using the
identity `iso(x) = x & -x`. The negation is computed in unsigned
arithmetic as `(0 - x)` mod 2^64, so no signed operation appears
anywhere. No builtins, no intrinsics, no tables.

## Contract decisions (pinned by dedicated test rows)

- `x = 0` maps to `0`. `(0 - 0)` is `0`, and `0 & 0` is `0`. The zero
  input is exercised by the exhaustive sweep (x = 0 is one of the
  65,536 cases) and asserted against the contract explicitly.
- Every nonzero input returns a single bit: the test asserts
  `iso & (iso - 1) == 0` and `popcount(x - iso) == popcount(x) - 1`
  for all 1,065,535 nonzero cases, using an independent naive
  bit-loop popcount, never a builtin.

## Verification

- Differential test against a naive per-bit scanning reference:
  all 65,536 exhaustive 16-bit inputs plus 1,000,000 fixed-seed
  splitmix64 64-bit values, zero mismatches.
- FNV-1a checksum of the full result stream is identical across
  -O0, -O2, and ASan+UBSan builds.
- Compiles clean under `-std=c11 -Wall -Wextra -Werror` with zero
  warnings; zero ASan/UBSan reports.
- gcc 13.3 -O2 emits exactly `mov; neg; and; ret` for
  `iso_lowest` (see PROOF.md): the and/sub construction, nothing
  extra.
