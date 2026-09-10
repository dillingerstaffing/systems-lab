# lab/77-bit-span

`bit_span(x)`: the position of the highest set bit of a 64-bit word
minus the position of the lowest set bit.  One number that says how
far apart a word's set bits stretch: a single-bit word has span 0, the
all-ones word has span 63.

## The identities

Two bit identities, each rebuilt from scratch in `bit_span.h`:

**floor_log2(x)**, the index of the highest set bit, from the shift/OR
fill propagation identity.  The cascade

    y = x;  y |= y >> 1;  y |= y >> 2;  y |= y >> 4;
    y |= y >> 8;  y |= y >> 16;  y |= y >> 32;

turns every bit at or below the highest set bit into 1, so `y` holds a
run of (p + 1) ones where `p` is the highest set bit index.  A
from-scratch SWAR popcount (pairwise sums, nibble sums, byte sums, the
multiply accumulating the eight byte counts into the top byte) counts
the run, minus 1 gives `p`.  Contract: `x != 0`.

**ctz(x)**, the index of the lowest set bit, from the de Bruijn
multiply-and-table identity.  `x & -x` isolates the lowest set bit as
a single-bit word `2^k`; multiplying it by the 64-bit de Bruijn
constant `0x03F79D71B4CB0A89` permutes the 64 single-bit words onto
distinct top-6-bit values, and `>> 58` reads the permutation key into
a 64-entry table.  The table is hardcoded in the header and the test
derives it independently from the rule `(1ULL << k) * C >> 58` and
checks all 64 entries.  Contract: `x != 0`.

`bit_span(x) = floor_log2(x) - ctz(x)`.

## The x = 0 contract

Both component identities are undefined at `x = 0` (the fill cascade
would underflow `popcount - 1`; `0 & -0` cannot index the table), so
`x = 0` is a separately documented row: `bit_span(0)` is defined as
`0`, since there are no set bits for the span to stretch across.  The
contract is pinned by a hand-checked anchor and by the exhaustive
16-bit pass, which includes 0.

## Verification

Differential-tested against two independent naive loop references (one
scanning from the top down for the highest set bit, one scanning from
the bottom up for the lowest set bit):

- 11 hand-checked anchors, including the `x = 0` contract row, single
  bits, all-ones, alternating patterns, and sparse words.
- All 65,536 16-bit inputs, exhaustively.
- Directed edge words: every single-bit position 0..63, an exact
  witness of every possible span value 0..63 (`(1ULL << hi) | 1`),
  plus all-ones / alternating / boundary patterns.
- 10,000,000 fixed-seed splitmix64 64-bit words
  (seed `0x123456789ABCDEF0`, the proof-engine convention).
- Both components checked against their own reference on every
  nonzero input, not just the difference.

Result: 10,065,685 cases, 0 mismatches.  The FNV-1a checksum of every
output is `0x7c36594632d8db46`, identical across the `-O0`, `-O2`, and
ASan+UBSan builds.  `-std=c11 -Wall -Wextra -Werror` clean; zero
sanitizer reports.  Throughput at `-O2`: 7.2 ns/value best of 5
(138.7 M values/s) over 100M timed values.  With the splitmix64 PRNG
step in the timed loop the cost rises to 10.6 ns/value, so roughly a
third of the timed loop is the PRNG; the 7.2 ns figure is the honest
per-value cost with the PRNG excluded.

No builtins, no intrinsics, no inline asm, no library calls in the
implementation.
