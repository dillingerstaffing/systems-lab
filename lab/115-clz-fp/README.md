# lab/115-clz-fp

Count leading zeros of a `uint64_t`, `clz64_fp(x)`, read off the
IEEE-754 binary64 exponent field.

The construction: fill `x` upward (`x |= x >> k` for k = 1, 2, 4, 8,
16, 32). For `x > 0` with `n = floor(log2(x))` the filled value is
exactly `2^(n+1) - 1`. Convert to `double` and read the exponent
field; the unbiased exponent is `floor(log2)` of the filled value,
so `clz = 63 - n`. Two facts from the bit layout make this exact:

- For `n <= 52` the filled value is below `2^53` and converts
  exactly, so the exponent field reads `n`.
- For `n >= 53` the conversion rounds up one exponent step and the
  field reads `n + 1`. At `n = 53` the filled value `2^54 - 1` is an
  exact midpoint between the doubles `2^54 - 2` (significand
  `1.111...1`, 52 ones, least bit odd) and `2^54` (significand
  `1.0`, least bit even); round-ties-to-even picks `2^54`. For
  `n >= 54` the filled value sits distance 1 below `2^(n+1)` and
  distance `2^(n-52) - 1 >= 3` above the next lower double, so it
  rounds up to `2^(n+1)`. The correction is one line: if
  `x >> 53` is nonzero, subtract 1 from the read exponent. The full
  derivation and the empirical exponent-read probe are in `PROOF.md`.

`x = 0` returns 64 by contract (handled explicitly; `(double)0`
has exponent field 0).

Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no
builtins, no library math in the implementation (the bits of the
`double` are read through a union; `__builtin_clzll` appears only
in the test oracle, never in `clz_fp.c`).

Verified in `test_clz_fp.c`:

- All 65,536 16-bit inputs, exhaustive, differential-checked
  against `__builtin_clzll`.
- The `x = 0` case checked separately against the 64 contract.
- 10,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked against
  `__builtin_clzll`.
- 10,065,537 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream: `bb15711279c99def`, identical across `-O0`,
  `-O2`, and ASan+UBSan builds.
- Measured at -O2: `14.29` ns/value (best of 5 reps over a 1M-value
  buffer).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory for the correctness builds, `make
bench` for throughput. The genuine build log and run output are in
`PROOF.md`.
