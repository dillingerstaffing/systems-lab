# lab/78-swar-max

`swar_max2_u16x4(x, y)`: the unsigned 16-bit maximum of each of four
packed lanes in a 64-bit word, branchless.  `swar_max_u16x4(x)`: the
maximum of the four lanes, as a `uint16_t`.

## The mechanism

One mechanism: a per-lane unsigned comparison built from a
subtraction whose guard bits make inter-lane borrows impossible.

**Step 1, the guarded subtraction.** With one guard bit per lane
(bit 15, `H = 0x8000800080008000`), `t = (x | H) - (y & ~H)` puts
the minuend lane in `[0x8000, 0xFFFF]` and the subtrahend lane in
`[0x0000, 0x7FFF]`.  A borrow out of a lane would need
`minuend < subtrahend + borrow_in <= 0x8000 <= minuend`, which is
impossible, so no borrow ever crosses a lane boundary and each
lane subtracts in isolation.  Bit 15 of lane `i` of `t` is then 1
exactly when the low 15 bits satisfy `lx_i >= ly_i`.

**Step 2, folding in bit 15.** Unsigned 16-bit comparison splits
on the top bits: `x_i >= y_i` exactly when `hx_i > hy_i`, or the
top bits agree and the low-15 comparison from step 1 holds.
`p = ((x & H) & ~(y & H)) | (~((x ^ y) & H) & (t & H))` carries
that predicate at bit 15 of each lane (the four `(hx, hy)` cases
are checked in the header comment).

**Step 3, mask expansion.** `m = p; m |= m >> 1; m |= m >> 2;
m |= m >> 4; m |= m >> 8` spreads the bit-15 predicate down to a
full `0xFFFF`/`0x0000` lane mask.  Right shifts by less than 16
cannot smuggle a bit from the lane above into bit 15 (the source
bit sits below that lane's set run, so it is 0), and every newly
set bit is the lane's own predicate copied downward.

**Step 4, select.** `(x & m) | (y & ~m)` keeps `x_i` where
`x_i >= y_i`, else `y_i`: the per-lane maximum.

**Horizontal max.** `m1 = max2(x, x >> 32)` folds lanes 2,3 onto
0,1; `m2 = max2(m1, m1 >> 16)` folds lane 1 onto lane 0, which
then holds the maximum of all four lanes.

## Verification

Differential-tested against scalar per-lane references:

- 11 hand-checked pairwise anchors plus 10 horizontal anchors,
  including the `0x7FFF`/`0x8000` unsigned boundary in every lane
  position and mixed-lane words; the horizontal anchors'
  expected values are cross-checked against the scalar
  reference so a typo cannot silently pass.
- All 2^32 `(a, b)` 16-bit pairs, exhaustively, replicated to all
  four lanes each iteration so every lane position sees the full
  pair space; each lane checked against scalar `max(a, b)`.
- Directed lane-crosstalk cases: for each lane position the test
  lane sweeps all 2^16 values while neighboring lanes sit at
  `0x0000`/`0xFFFF`/`0x7FFF`/`0x8000` in one word and the
  complementary pattern in the other (4,194,304 cases).
- Horizontal max: 14 directed words (max planted in each lane
  position, ties, extremes, monotone patterns) plus 10,000,000
  fixed-seed splitmix64 64-bit words
  (seed `0x123456789ABCDEF0`, the proof-engine convention).

Result: 4,309,161,635 cases, 0 mismatches.  The FNV-1a checksum of
every output is `0x64d1c937981935ce`, identical across the `-O0`,
`-O2`, and ASan+UBSan builds.  `-std=c11 -Wall -Wextra -Werror`
clean; zero sanitizer reports.  Throughput at `-O2`, best of 5
over 100M timed values with the splitmix64 PRNG pre-generated
and excluded from the timed loop: horizontal max 3.943 ns/value
(253.589 M values/s), pairwise core 1.484 ns/value (673.973 M
values/s); with the PRNG inside the timed loop the horizontal
cost is 12.927 ns/value.  Exact logs are in PROOF.md.

No builtins, no intrinsics, no inline asm, no library calls in
the implementation.
