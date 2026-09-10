# lab/97-parity-6996

Parity of a 32-bit word, `parity32(x)`: 1 if `x` has an odd number of
set bits, 0 otherwise, computed in two steps.

First, the word is folded to one nibble with `x ^= x >> 16`,
`x ^= x >> 8`, `x ^= x >> 4`. Parity is the XOR of all 32 bits, and
XOR is associative and commutative, so each fold preserves the XOR
of all bits in the surviving half; after the three folds the low
nibble's parity equals the original word's parity.

Second, the folded nibble's parity is read from the constant
`0x6996`, where bit i of the constant equals the parity of the 4-bit
value i (hand derivation in `PROOF.md`). So
`(0x6996u >> (x & 0xf)) & 1u` yields the answer. No library
popcount or parity builtin is used.

Verified in `test_parity.c`:

- All 65,536 16-bit inputs, exhaustive, differential-checked against
  a naive per-bit-loop reference.
- 1,000,000 fixed-seed `splitmix64` 32-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked against the same
  reference.
- The homomorphism `parity(a ^ b) == parity(a) ^ parity(b)` on
  1,000,000 pairs from a distinct fixed seed (like `lab/35-parity-fold`).
- 2,065,536 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream: `c934e844f0908c2e`, identical across `-O0`,
  `-O2`, and ASan+UBSan builds.
- The `-O2` disassembly keeps the shift/XOR fold and the
  `$27030` (= 0x6996) constant with a `shr`; no popcount-class
  instruction is emitted.
- Measured at -O2: 3.44 ns/value (290.4 Mvalues/s over 25M timed
  values, best of 5).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
