# lab/103-divmod-pow2-neg

`divmod_pow2(x, k, &q, &r)` returns the quotient and remainder of a
signed 64-bit `x` divided by `2^k` in one pass, with no `/` or `%`
anywhere in the implementation.

## The two identities

Quotient: `q = (x + ((x >> 63) & (2^k - 1))) >> k`. An arithmetic
right shift of a negative value rounds toward negative infinity; C `/`
rounds toward zero. `x >> 63` is all ones exactly when `x` is
negative, so the AND selects a positive bias of `2^k - 1` only for
negative inputs, correcting the rounding before the shift.

Remainder: `r = x - (q << k)`. The truncated multiple is rebuilt from
the quotient and subtracted back out. The shift is done in unsigned
arithmetic so a negative quotient never triggers a signed left shift;
the true remainder lies in `(-2^k, 2^k)`, so re-reading the wrapped
bits as `int64_t` is exact.

## Contract decisions (pinned by dedicated test rows)

- `k` in `[1, 63]`. `k = 0` is the identity (`q = x`, `r = 0`) and
  `k = 64` is not representable as a shift width; both are outside
  the contract rather than handled specially.
- `x != INT64_MIN`. Its magnitude is not representable, so the
  sign-corrected identity is contracted over the domain where `|x|`
  is representable. The test domain (all int16_t values plus 64-bit
  edges including `INT64_MIN + 1`) covers the contract fully.
- `r` is zero or carries the sign of `x` (C `%` semantics), `|r| < 2^k`,
  and `q * 2^k + r == x` exactly.

## Verification

- Differential test against C `/` and `%` (reference only, never in
  the implementation): all 65,536 exhaustive 16-bit inputs for every
  `k = 1..63`, plus 11 fixed 64-bit edge values per `k`, for
  4,129,461 checks with zero mismatches.
- Per case, the invariants `q * 2^k + r == x` (exact, `__int128`),
  sign agreement, and `|r| < 2^k` are asserted independently of the
  reference operators.
- FNV-1a checksum of the full output stream is identical across
  `-O0`, `-O2`, and ASan+UBSan builds.
- Compiles clean under `-std=c11 -Wall -Wextra -Werror` with zero
  warnings; zero ASan/UBSan reports.
- gcc 13.3 `-O2` emits exactly `shl; sar; not; and; add; sar; mov;
  shl; sub; mov; ret` for `divmod_pow2` (see PROOF.md): the two
  identities, no `div`/`idiv` anywhere in the object.
- A signed left shift of the negative quotient was caught by UBSan
  during development and moved to unsigned arithmetic; the sanitizer
  build has been clean since.
