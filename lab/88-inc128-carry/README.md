# lab/88-inc128-carry

128-bit increment with carry cascade, `inc128(x)`: the 128-bit
value is held as two `uint64_t` words `(hi, lo)`. Adding 1 computes
`lo2 = lo + 1`; the carry into the high word is 1 exactly when `lo2`
wrapped to 0, i.e. exactly when `lo` was `UINT64_MAX` (unsigned
addition wraps modulo 2^64, C11 6.2.5p9). Then `hi2 = hi + carry`,
and the carry out of bit 127 is 1 exactly when the whole input was
all ones, which is exactly when `lo2 == 0 && hi2 == 0` (full
derivation in `PROOF.md`).

The `unsigned __int128` type appears only in the test oracle, never
in the implementation; no intrinsics or builtins are used anywhere.

Verified in `test_inc128.c`:

- Directed edge rows: `(0,0)`, `(0,UINT64_MAX)`, `(UINT64_MAX,0)`,
  `(UINT64_MAX,UINT64_MAX)`, `(1,UINT64_MAX)`, each
  differential-checked against the oracle and printed.
- All 2^32 pairs `(hi, lo)` with `hi, lo` in `[0, 2^16)`, exhaustive,
  differential-checked against an `unsigned __int128` oracle
  (`ref = (unsigned __int128)x + 1` wrapping mod 2^128,
  `carry_ref = (x == 2^128 - 1)`): `new_hi == ref >> 64`,
  `new_lo == (uint64_t)ref`, `carry_out == carry_ref` on every case.
- 1,000,000 fixed-seed `splitmix64` random 64-bit pairs (seed
  `0x123456789ABCDEF0`), same differential check on every case.
- 4,295,967,301 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream: `e7c096581363da96`, identical across `-O0`, `-O2`,
  and ASan+UBSan builds.
- Measured at -O2: 4.85 ns/value (206.1 Mvalues/s over 100M
  timed values, best of 5; inputs from splitmix64 inside the timed
  loop).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
