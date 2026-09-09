# lab/49-div-round-pow2: round-to-nearest division of u32 by 2^k

`round_half_up_div_pow2()` (divround.h) computes the nearest integer to
`x / 2^k` from the identity `(x + 2^(k-1)) >> k`: adding half the divisor
before the shift moves the rounding boundary so an exact fractional 1/2
rounds up and anything below keeps its floor value.

Two shift-count edge cases never execute an invalid shift:

- `k = 0` is division by 1: returns `x` unchanged, no shift executed.
- `k >= 32` is computed in 64 bits as `((uint64_t)x + 2^31) >> k`
  (shifting a 64-bit operand by 32 is valid); for 32-bit input the exact
  quotient is below 1, so the result is 1 exactly at the tie `x = 2^31`
  and 0 otherwise.

The 64-bit intermediate is also what makes the core identity exact over
the whole u32 domain: the 32-bit form of `x + 2^(k-1)` wraps for large
inputs (e.g. `x = 0xFFFFFFFF, k = 31` gives 0 instead of 2 with 32-bit
arithmetic), so the module adds in a wider register and narrows the
result.

## Files

- `divround.h`: the implementation (one function).
- `test_divround.c`: 17 hand-checked known-answer vectors (ties at every
  level, both edge cases, the 32-bit-wrap counterexample), an exhaustive
  differential test over all 16-bit inputs for k=1..15, a 1,000,000-sample
  random differential test over the full u32 range for k=1..31 (splitmix64,
  fixed seed 0x243F6A8885A308D3), an FNV-1a checksum over all results, and
  a throughput benchmark.
- `Makefile`: `test-o0`, `test-o2`, `test-asan`, `test-ubsan`, `clean`.
- `PROOF.md`: genuine build log and measured numbers.

## Measured

- 17/17 vectors pass, including `x=3,k=1 -> 2` (1.5 rounds up),
  `x=5,k=2 -> 1` (1.25 rounds down), ties `x=2^(k-1) -> 1` at k=1, 7, and
  32, `k=0` identity on 0xDEADBEEF, and the 64-bit-add cases
  `x=0xFFFFFFFF,k=31 -> 2` and `x=0xFFFFFFFF,k=1 -> 0x80000000`.
- Differential: 983,040 exhaustive checks (all x in 0..65535, k=1..15) and
  1,000,000 random checks (full u32 x, k=1..31) against an independent
  reference `(2x + 2^k) / 2^(k+1)` built from multiply/add/divide only:
  1,983,040 total checks, 0 mismatches.
- FNV-1a fingerprint 0xee4a225fd47d1345 over all results, identical across
  -O0, -O2, ASan+UBSan, and UBSan builds.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` at -O0, -O2, and
  both sanitizer builds; zero warnings, zero sanitizer reports (built
  with `-fno-sanitize-recover=all`, so any report would be fatal).
- Throughput at -O2: 40,000,000 calls in 0.195 s = 4.9 ns/value. The
  timed loop is `sink += round_half_up_div_pow2((uint32_t)i, 1+(i%31))`;
  this is a ceiling on the true per-call cost, not a floor. See PROOF.md
  for the full build and run logs.
