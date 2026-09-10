# lab/58-signed-div-pow2: truncated signed division of int64 by 2^k

`sdiv_trunc_pow2()` (sdivpow2.h) returns the same value C computes for
`x / 2^k` (truncation toward zero), from the shift identity
`(x + ((x >> 63) & (2^k - 1))) >> k`.

The bias exists because an arithmetic right shift of a negative value
rounds toward negative infinity while C division rounds toward zero.
For `x < 0`, write `x = q * 2^k + r` with `q = floor(x / 2^k)` and
`0 <= r < 2^k`. Adding `2^k - 1` gives `(q+1) * 2^k + (r-1)` when
`r > 0` and `q * 2^k + (2^k - 1)` when `r = 0`, so the shift yields
`q + 1` or `q` respectively, which is exactly `trunc(x / 2^k)` in both
cases. For `x >= 0` the bias is 0. The signed addition cannot overflow:
for `x < 0` and `k <= 63`, `x + (2^k - 1)` stays inside
`[-2^63, 2^63 - 2]`; the UBSan build checks this on every input.

Edge cases, both without any invalid shift:

- `k = 0` is division by 1: returns `x` unchanged, no shift executed.
- `k >= 64`: `|x| <= 2^63 < 2^k`, so the truncated quotient is exactly 0.

Note on naming: this is C truncation semantics, not mathematical floor
division. For negative non-multiples the two differ (e.g. `x = -7, k = 1`
gives -3 here, while floor would give -4); the differential test is
against C's `/` operator, which is what the identity computes.

## Files

- `sdivpow2.h`: the implementation (one function).
- `test_sdivpow2.c`: 17 hand-checked known-answer vectors (negative
  non-multiples where a bare shift would round the wrong way,
  `INT64_MIN`/`INT64_MAX` at `k = 1` and `k = 63`, `k = 0` identity,
  `k = 64` below-one quotients), an exhaustive differential test over
  every 16-bit signed input for `k = 1..15` against plain C division,
  a 1,000,000-sample random differential test over full 64-bit inputs
  for `k = 1..63` (splitmix64, fixed seed 0x243F6A8885A308D3), an
  FNV-1a checksum over all results, and a throughput benchmark.
- `Makefile`: `test-o0`, `test-o2`, `test-asan`, `test-ubsan`, `clean`.
- `PROOF.md`: genuine build log and measured numbers.

## Measured

- 17/17 vectors pass, including `x=-7,k=1 -> -3` (bare `>>` gives -4),
  `x=-7,k=2 -> -1` (bare `>>` gives -2), `x=-1,k=1 -> 0` (bare `>>`
  gives -1), `x=INT64_MIN,k=63 -> -1`, `x=INT64_MAX,k=63 -> 0`,
  `k=0` identity on +-5, and `k=64` returning 0.
- Differential: 983,040 exhaustive checks (all x in -32768..32767,
  k=1..15) and 1,000,000 random checks (full int64 x, k=1..63) against
  independent C division `x / 2^k`: 1,983,057 total checks, 0 mismatches.
- FNV-1a fingerprint 0x3fbd3c3962be4d43 over all results, identical
  across -O0, -O2, ASan+UBSan, and UBSan builds.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` at -O0, -O2, and
  both sanitizer builds; zero warnings, zero sanitizer reports (built
  with `-fno-sanitize-recover=all`, so any report would be fatal). The
  sanitizer runs additionally confirm the `x + bias` addition never
  overflows, including on `INT64_MIN`.
- Throughput at -O2: 40,000,000 calls in 0.092 s = 2.3 ns/value. The
  timed loop varies x and k each iteration; this is a ceiling on the
  true per-call cost, not a floor. See PROOF.md for the full build and
  run logs.
