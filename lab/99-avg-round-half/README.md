# lab/99-avg-round-half

Round-half-up average of two `uint64_t` values, `avg_rhu_u64(a, b)`:
`rhu((a + b) / 2)`, computed without ever forming `a + b`.

Companion to `lab/59-avg-no-overflow` (which rounds down). This one
rounds half up: for each bit position the exact identity
`a + b = 2 * (a & b) + (a ^ b)` holds, so with `y = a & b` and
`x = a ^ b` the exact average is `y + x/2`. Rounding half up adds 1
exactly when `x` is odd, i.e. exactly when the exact sum is odd (the
`.5` case), giving `y + (x >> 1) + (x & 1)`.

Why the extra term cannot overflow: `y + (x >> 1)` equals
`floor((a + b) / 2)` exactly, so it is at most `2^64 - 1`. When the
correction `(x & 1)` is 1 the sum is odd, and the largest odd sum is
`2^65 - 3` (the maximum sum `2^65 - 2` is even), so the floor part is
at most `2^64 - 2` in that case and the final `+1` stays below `2^64`.
No signed arithmetic and no shift of a negative anywhere; plain C11,
`-std=c11 -Wall -Wextra -Werror`, no builtins (the `__int128` oracle
lives in the test only, never in the implementation).

Verified in `test_avg_round_half.c`:

- All 2^32 pairs of 16-bit operands, exhaustive, differential-checked
  against the `__int128` exact oracle.
- 13 directed tie rows (odd-sum cases where the exact average ends in
  `.5`, including `(0,1)`, `(UINT64_MAX,0)`,
  `(UINT64_MAX,UINT64_MAX-1)`, `(2^64-1,2^64-1)`), printed as a table
  pinning the tie behavior; each row differential-checked.
- 10,000,000 fixed-seed `splitmix64` 64-bit pairs (seed
  `0x123456789ABCDEF0`), each differential-checked.
- 4,304,967,309 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream: `1f09d1d42620ee61`, identical across `-O0`, `-O2`, and
  ASan+UBSan builds.
- Measured at -O2: `10.11` ns/value (best of 5, each value draws two
  64-bit splitmix64 operands).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
