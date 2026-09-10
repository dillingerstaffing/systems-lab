# lab/95-swar-hsum

Horizontal sum of four packed unsigned 16-bit lanes in one 64-bit
word: `swar_hsum(w)` returns lane0+lane1+lane2+lane3 (at most
262140), computed by the pairwise-add cascade identity. Stage 1 sums
the even lanes with the odd lanes shifted down (`w & M` plus
`(w >> 16) & M`, `M = 0x0000FFFF0000FFFF`), leaving lane pair sums in
32-bit fields; stage 2 folds the two fields into the total.

The absence of lane crosstalk is proved from the guard bounds, not
from testing: each lane holds at most 0xFFFF, so each pair sum is at
most 0x1FFFE, strictly less than 2^32, and therefore cannot produce
a carry into the next 32-bit field. The final fold is at most
0x3FFFC, below 2^32, so it is exact with no carry out. The full
derivation is in `PROOF.md`.

Verified in `test_hsum.c` against an independent scalar
lane-shift-and-add reference:

- Directed rows: 14 words covering the zero word, the all-ones word,
  each lane alone at maximum, adjacent pair boundaries, the all-0x8000
  case (pair sums at exactly 0x10000, the sharpest guard-bound case),
  and arbitrary patterns, each checked against the reference and
  printed in the log.
- 4,294,967,296 exhaustive words of the form (a, b, a, b): every
  16-bit lane pair replicated to all four lanes, differential-checked
  against the reference.
- 10,000,000 fixed-seed `splitmix64` random 64-bit words (seed
  `0x123456789ABCDEF0`), each differential-checked against the same
  reference.
- 4,304,967,310 total checks, 0 mismatches. FNV-1a 64-bit checksum
  over the result stream: `0eb4f4d7484f98bd`, identical across `-O0`,
  `-O2`, and ASan+UBSan builds.
- Measured at -O2: 2.21 ns/value (452.1 Mvalues/s over 25M timed
  values, best of 5; the PRNG is outside the timed loop).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory. The genuine build log and run
outputs are in `PROOF.md`.
