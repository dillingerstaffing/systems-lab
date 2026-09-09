# lab/35-parity-fold

Parity of a 64-bit word, `parity64(x)`: 1 if `x` has an odd number of
set bits, 0 otherwise, computed by the xor-fold reduction.

The fold works because the XOR of all bits of a word equals its
parity. XOR is associative and commutative, so folding the high half
into the low half (`x ^= x >> k`) leaves the XOR of all bits
unchanged: with x = hi:lo, the low half becomes hi ^ lo, and
parity(hi ^ lo) = parity(hi) ^ parity(lo) = parity(x). Repeating for
k = 32, 16, 8, 4, 2, 1 reduces the word until the XOR of the original
64 bits sits in bit 0. `parity.c` is built only from this reduction;
no library popcount or parity builtin is used.

Verified in `test_parity.c`:

- All 65,536 16-bit inputs, exhaustive, differential-checked against
  a naive per-bit-loop reference.
- 1,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked against the same
  reference.
- The homomorphism `parity(a ^ b) == parity(a) ^ parity(b)` on
  1,000,000 pairs from a distinct fixed seed.
- 2,065,536 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream, identical across `-O0`, `-O2`, and ASan+UBSan
  builds.
- Measured at -O2: 4.26 ns/value (234.6 Mvalues/s over 25M timed
  values).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
