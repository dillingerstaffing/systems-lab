# lab/87-branchless-sign

Sign of a signed 64-bit value, `sign64(x)`: -1 if `x < 0`, 0 if
`x == 0`, +1 if `x > 0`, computed with no comparison and no branch.

The implementation reads the answer out of the two's-complement bit
pattern directly. `(uint64_t)x >> 63` is the sign bit of `x`; negated
as an `int64` it is -1 for negative `x` and 0 otherwise. The positive
case comes from the top bit of the unsigned negation `0u - (uint64_t)x`,
which wraps modulo 2^64 and so is well-defined even for
`x = INT64_MIN` (where the signed negation `-x` would be undefined
behavior); that top bit is 1 exactly when `x > 0`. OR-ing the two
parts gives -1, 0, or +1 for every `int64`. The full hand derivation,
including the INT64_MIN contract, is in `PROOF.md`.

Verified in `test_sign.c`:

- Directed rows: `INT64_MIN`, `-1`, `0`, `1`, `INT64_MAX`, each checked
  against a comparison-based reference and printed in the log.
- All 65,536 16-bit inputs as `int64`, exhaustive, differential-checked
  against the reference `(x > 0) - (x < 0)`.
- 1,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked against the same
  reference.
- 1,065,541 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream: `21a06a0e15a1874e`, identical across `-O0`, `-O2`,
  and ASan+UBSan builds.
- The `-O2` disassembly of `sign64` contains no jump instruction at
  all (`endbr64`, `mov`, `sar`, `neg`, `shr`, `or`, `ret` only); the
  full excerpt is in `PROOF.md`.
- Measured at -O2: 2.22 ns/value (450.7 Mvalues/s over 25M timed
  values, best of 5).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
