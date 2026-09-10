# lab/114-swar-eq-mask

Bytewise equality mask of two 64-bit words: `swar_eqmask64(x, y)`
returns an 8-bit mask with bit `i` set iff byte `i` of `x` equals
byte `i` of `y` (byte 0 is the least significant byte), computed
with no conditional branch in the implementation.

The implementation starts from `d = x ^ y` (byte `i` of `d` is zero
exactly when the bytes agree) and the zero-byte detection identity
`t = ((d - 0x0101010101010101) & ~d & 0x8080808080808080)`. The
naive form of that identity is only an existence test: it flags a
nonzero `0x01` byte that receives a borrow from below (e.g.
`d = 0x0100`), which the differential test below caught on the
first run. The shipped code therefore tests even and odd bytes
separately, parking the other half at `0xFF`, which can never
borrow out (`0xFF - 1 - c >= 0xFD`), so every tested byte sees
borrow-in 0 and the per-byte zero test is exact: `0x80` in byte
`i` iff byte `i` of `d` is zero, no false positives and no false
negatives. The eight `0x80` bits are gathered with
`u = t >> 7` and `((u >> 7i) & (1 << i))` per byte, ORed into the
low byte. All arithmetic is unsigned, all shifts are compile-time
constants in `[0, 64)`. The full hand derivation, including the
false-positive analysis, is in `PROOF.md`.

Verified in `test_eqmask.c`:

- Directed rows: 16 `(x, y)` pairs covering all-equal,
  all-different, single-byte differences at the low end, the high
  end, and the middle, the `0x80` high-bit boundary, and the
  adversarial `d = 0x0101010101010101` input, each checked against
  the naive per-byte-loop reference and printed in the log.
- 4,294,967,296 exhaustive 16-bit `(x, y)` pairs (`x` and `y` over
  `[0, 65535]`), differential-checked against the reference.
- 10,000,000 fixed-seed `splitmix64` random 64-bit pairs (seed
  `0x123456789ABCDEF0`), each differential-checked against the
  same reference.
- 4,304,967,312 total checks, 0 mismatches. FNV-1a 64-bit checksum
  over the result stream: `731e0818deb17926`, identical across `-O0`,
  `-O2`, and ASan+UBSan builds.
- The `-O2` disassembly of `swar_eqmask64` contains no conditional
  jump (programmatic scan: 0 jump instructions; the compiler folds
  both subtractions into `lea`/`add` against `-0x0101010101010101`
  and reduces the mask logic to `not`/`and`/`or`/`shr` chains).
  The full excerpt is in `PROOF.md`.
- Measured at -O2: 4.31 ns/value (232.1 Mvalues/s over 25M timed
  values, best of 5).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
