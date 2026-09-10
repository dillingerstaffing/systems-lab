# lab/91-div3-magic

Unsigned 32-bit division by 3 with no division operator in the
implementation. `div3_u32(x)` computes

    q = (uint32_t)(((uint64_t)x * 0xAAAAAAABULL) >> 33)

a single widening multiply and a shift. The constant is
M = 0xAAAAAAAB = ceil(2^33 / 3) = (2^33 + 1) / 3. Writing
x = 3a + r with r in {0, 1, 2} gives x * M = a * 2^33 + a +
r * (2^33 + 1) / 3, and the term after a * 2^33 is strictly
below 2^33 for every x in [0, 2^32) (a < 2^31, so a / 2^33 < 1/6,
and the r term is below 2/3 + 1/2^32), so the shift by 33
leaves exactly a = floor(x / 3). The product x * M is below
2^64, so the `uint64_t` multiply is exact. Full derivation in
`PROOF.md`.

Verified in `test_div3.c`:

- 19 directed edge rows (0, residue classes mod 3 at both ends
  of the range, powers-of-two boundaries), each printed and
  differential-checked against native `/ 3`.
- All 65,536 16-bit inputs, exhaustive, differential-checked
  against native `/ 3` (the oracle lives only in the test file).
- 1,000,000 fixed-seed `splitmix64` random 32-bit values (seed
  `0x123456789ABCDEF0`), same differential check.
- 1,065,555 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream: `b2e1fc5468e18c88`, identical across `-O0`,
  `-O2`, and ASan+UBSan builds.
- gcc 13.3 `-O2` disassembly of `div3_u32` is `mov / imul / shr /
  ret` (plus `endbr64`): no `div` or `idiv` instruction.
- Measured at -O2: 2.221 ns/value (450.3 Mvalues/s over 100M
  timed values, best of 5; inputs pre-generated from splitmix64
  into a 1M-entry array, excluded from the timed loop).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
