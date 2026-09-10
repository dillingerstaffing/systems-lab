# lab/89-swar-abs

`swar_abs_u16x4(x)`: the signed 16-bit absolute value of each of four
packed lanes in a 64-bit word, branchless.  Contract: `abs(-32768)`
wraps to `-32768` (`0x8000`), pinned and tested.

## The mechanism

One mechanism: the sign-mask identity `abs = (x ^ m) - m` evaluated
per lane with a guard bit that makes inter-lane borrows impossible,
plus a final mask that strips the injected guard bit.

**Step 1, the sign mask.** `m = x & H` keeps each lane's sign bit,
then `m |= m >> 1; m |= m >> 2; m |= m >> 4; m |= m >> 8` spreads it
to a full `0xFFFF`/`0x0000` lane mask.  Right shifts by less than 16
cannot smuggle a bit from the lane above into bit 15 (the source
bit sits below that lane's set run, so it is 0), so no bit ever
crosses a lane boundary.

**Step 2, the guarded subtraction.** `t = ((x ^ m) | H) - (m & ~H)`.
Lane `i`'s minuend `(x_i ^ m_i) | 0x8000` lies in `[0x8000, 0xFFFF]`
(the XOR always clears bit 15: a non-negative lane has bit 15 = 0,
a negative lane has it flipped to 0) and the subtrahend lane
`(m_i & 0x7FFF)` is 0 or `0x7FFF`.  A borrow out of a lane would need
`minuend < subtrahend + borrow_in <= 0x8000 <= minuend`, which is
impossible, so by induction no borrow ever leaves a lane and each
lane computes `(x_i ^ m_i) - m_i` mod 2^16 in isolation, plus an
injected `0x8000` exactly in the non-negative lanes.

**Step 3, strip the guard.** For a non-negative lane
`t_i = x_i + 0x8000` and clearing bit 15 recovers `x_i`; for a
negative lane `t_i = (0xFFFF - x_i) + 1`, the two's-complement
negation, whose bit 15 is genuine result data.  `r = t & (m | ~H)`
clears bit 15 exactly in the non-negative lanes and keeps every
bit in the negative lanes.

The `-32768` wrap is a pinned contract: the lane's
two's-complement negation mod 2^16 of `0x8000` is `0x8000`, so
`abs(-32768) = -32768`, the same wrap the scalar reference
produces through unsigned arithmetic.

## Verification

Differential-tested against a scalar per-lane reference (a plain
if/else on extracted `int16_t` lanes, not the bit trick):

- 48 hand-checked anchors: 10 per-lane values (`0, 1, 0x7FFE,
  0x7FFF, 0x8000, 0x8001, 0xFFFE, 0xFFFF, 0x1234, 0xEDCC`)
  planted in every lane position plus 8 mixed-lane words; every
  expected value cross-checked against the scalar reference so a
  typo cannot silently pass.
- Exhaustive: all 2^16 lane values x all four lane positions
  (neighbors zero), plus the same sweep with neighbors at
  `0x0000`/`0xFFFF`/`0x7FFF`/`0x8000` (1,310,720 cases).
- Directed lane-crosstalk: alternating sign patterns across lanes,
  `-32768` in each lane position with neighbors at every
  combination of `32767`/`-1`/`0`, and all 64 single-bit words
  (180 cases).
- 10,000,000 fixed-seed splitmix64 64-bit words
  (seed `0x123456789ABCDEF0`, the proof-engine convention).
- On every case, the invariants `abs(abs(x)) == abs(x)` and "each
  result lane is non-negative, except a `0x8000` result lane whose
  input lane was `0x8000`" held with 0 violations.

Result: 11,310,948 cases, 0 mismatches.  The FNV-1a checksum of
every output is `0xd2d96df3ef5780e2`, identical across the `-O0`,
`-O2`, and ASan+UBSan builds.  `-std=c11 -Wall -Wextra -Werror`
clean; zero sanitizer reports.  Disassembly check
(`gcc 13.3.0 -O2`, non-inline wrapper, `objdump -d`): the function
body is `and`/`shr`/`or`/`xor`/`sub`/`ret` with 0 conditional jump
instructions and no `cmov`.  Throughput at `-O2`, best of 5 over
100M timed values with the splitmix64 PRNG pre-generated and
excluded from the timed loop: 1.210 ns/value (826.673 M values/s);
with the PRNG inside the timed loop the cost is 5.650 ns/value.
Exact logs are in PROOF.md.

No builtins, no intrinsics, no inline asm, no library calls in
the implementation.
