# lab/51-branchless-abs-diff

`badiff64`: branchless absolute difference of two `int64_t` values,
returned as a `uint64_t`. The sign-mask identity
`(d ^ (d >> 63)) - (d >> 63)` is applied to the difference `d = a - b`
entirely on unsigned 64-bit values, so no signed overflow or
implementation-defined shift is possible anywhere in the function.
Header-only: `absdiff.h`. Tests: `test_absdiff.c`.

- Contract: defined exactly when the true difference satisfies
  `INT64_MIN < (a - b) <= INT64_MAX`. The one excluded input,
  `d == INT64_MIN`, is documented in `PROOF.md` (the signed negation
  would overflow; the reference `llabs` is undefined there) and is
  never exercised by the test.
- `test_absdiff.c`
  - Exhaustive over all 2^16 x 2^16 = 4,294,967,296 `int16_t` pairs,
    sign-extended into `int64_t`, differential-checked against
    `llabs(d)`.
  - Directed 81-pair edge sweep over
    `{INT64_MIN, INT64_MIN+1, -2^60, -1, 0, 1, 2^60, INT64_MAX-1,
    INT64_MAX}`; the exact difference is taken in `__int128` and the
    24 pairs outside the contract (including the `(INT64_MIN, 0)`
    exclusion) are skipped and counted, independently recomputed in
    `PROOF.md`.
  - 1,000,000 fixed-seed splitmix64 random pairs with each operand in
    `[-2^60, 2^60)`, so every difference fits by construction.
  - Total: 4,295,967,353 differential checks, 0 mismatches, at `-O0`,
    `-O2`, and under ASan+UBSan; identical checksum sink
    (`0xb2fe6b37b79e710f`) in all three timed loops.
  - Disassembly check (`gcc -O2`, non-inline wrapper, `objdump -d`):
    the body is `sub`/`sar`/`xor`/`sub` with no conditional jump and
    no `cmov`.
  - Benchmark: timed loop of 200,000,000 pairs at `-O2`, reported in
    ns/pair in PROOF.md (4.389 ns/pair).
