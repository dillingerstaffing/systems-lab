# lab/45-hamming-dist

Hamming distance of two 64-bit words, `d(a, b) = popcount(a ^ b)`.

`a ^ b` has exactly the bits set where `a` and `b` differ, and counting
those bits is the distance. The count is done by the SWAR parallel-add
identities from `lab/17-bitcount`, rebuilt inside `hamming.c` so this
module stands alone: pairwise 1-bit adds, then 2-bit, then 4-bit, with
masks enforcing the field boundaries, and the final multiply-shift fold
summing the eight byte counters. No library popcount is wrapped.

Verified in `test_hamming.c`:

- 73 directed edge cases: equal values give 0, all-ones vs zero gives
  64, all 64 single-bit flips of a base word give 1, alternating words
  vs their opposites give 64, `d(x, ~x) == 64` for four fixed words, and
  the symmetry invariant `d(a,b) == d(b,a)` on every case.
- 10,000,000 fixed-seed `splitmix64` pairs (seed
  `0x123456789ABCDEF0`), each distance differential-checked against a
  naive bit-loop reference, 0 mismatches. 10,000,073 total checks.
- FNV-1a 64-bit checksum over the distance stream, identical across
  `-O0`, `-O2`, and ASan+UBSan builds.
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run output
are in `PROOF.md`.
