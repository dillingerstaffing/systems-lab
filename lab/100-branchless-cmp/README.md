# lab/100-branchless-cmp

Branchless three-way unsigned 64-bit comparison: `bcmp64(a, b)`
returns `-1`, `0`, or `+1` for `a < b`, `a == b`, `a > b`,
computed from the borrow-out identity with no conditional branch
in the implementation.

The implementation rests on one unsigned fact: for unsigned `a`,
`b`, `(a - b) mod 2^64 > a` iff `a < b` (C11 6.2.5p9: unsigned
subtraction wraps, so it is total; the proof splits on `a >= b`,
where `a - b` is in `[0, a]`, versus `a < b`, where the wrapped
difference is `2^64 - (b - a) > a` because `2^64 - b >= 1`). The
`a == b` flag is plain unsigned equality. `lt` and `eq` cannot both
be 1, so `gt = 1 - lt - eq` is exactly the `a > b` flag, and
`gt - lt` is `-1`, `0`, or `+1`. The only signed operations are the
flag conversions and that final subtraction, confined to
`{-1, 0, +1}`, so no overflow or undefined behavior is possible.
The full hand derivation is in `PROOF.md`.

Verified in `test_cmp.c`:

- Directed rows: 14 `(a, b)` pairs covering 0, `UINT64_MAX`,
  neighbors on both sides, the `0x8000000000000000` vs
  `0x7FFFFFFFFFFFFFFF` boundary, and mixed patterns, each checked
  against the reference `(a > b) - (a < b)` and printed in the log.
- 4,294,967,296 exhaustive 16-bit `(a, b)` pairs (`a` and `b` over
  `[0, 65535]`), differential-checked against the reference.
- 10,000,000 fixed-seed `splitmix64` random 64-bit pairs (seed
  `0x123456789ABCDEF0`), each differential-checked against the
  same reference.
- 4,304,967,310 total checks, 0 mismatches. FNV-1a 64-bit checksum
  over the result stream: `07ae4885650dd8ca`, identical across
  `-O0`, `-O2`, and ASan+UBSan builds.
- The `-O2` disassembly of `bcmp64` contains no conditional jump
  (programmatic scan: 0 jump instructions; `endbr64`, `cmp`,
  `setb`, `sete`, `movzbl`, `add`, `sub`, `ret` only); the compiler
  folds the subtraction away and reads the borrow directly from
  the hardware carry flag via `setb`. The full excerpt is in
  `PROOF.md`.
- Measured at -O2: 2.31 ns/value (432.7 Mvalues/s over 25M timed
  values, best of 5).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
