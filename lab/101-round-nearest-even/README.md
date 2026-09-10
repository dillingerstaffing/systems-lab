# lab/101-round-nearest-even

Round a 64-bit Q32.32 fixed-point value (32 integer bits, 32
fraction bits) to the nearest integer with ties to even, using
only bit-manipulation identities. No float, no division anywhere
in `round_rne.c`.

The construction:

    round_bit = fraction bit 31 (value exactly 1/2)
    sticky    = OR of fraction bits 0..30 (1 iff the fraction is
                strictly off the half boundary)
    lsb       = integer bit 0
    round_up  = round_bit AND (sticky OR lsb)
    result    = integer_part + round_up

Case analysis on the fraction bits (bit k of the fraction has
value 2^(k-32)):

- `round_bit = 0`: the fraction is below 1/2, because the top
  fraction bit alone is worth 1/2 and bits 0..30 sum to at most
  2^31 - 1 units of 2^-32, which is strictly less than 1/2. So
  `round_up = 0` and the result is the integer part.
- `round_bit = 1, sticky = 1`: the fraction is 1/2 plus at least
  one lower bit, so strictly above 1/2. `round_up = 1`.
- `round_bit = 1, sticky = 0`: the fraction is exactly 1/2. The
  candidates are the integer part and integer part + 1; exactly
  one is even, and it is the integer part when `lsb = 0`,
  integer part + 1 when `lsb = 1`. So `round_up = lsb` picks the
  even neighbor. This is the ties-to-even rule.

The final addition is unsigned 32-bit. Integer part
`0xFFFFFFFF` with `round_up = 1` yields 0: the wraparound
contract (0xFFFFFFFF.FFFFFFFF rounds up and wraps to 0),
documented and pinned by dedicated test rows.

Scope note: lab/28-tie-to-even-half rounds only `x / 2` (a single
halving, where the remainder bit is the whole decision). This
module is the general Q32.32 case: the fraction is a full 32-bit
field, the midpoint is an interior value (bit 31 of the
fraction), and the sticky OR over the lower 30 bits is the part
lab/28 never needs. Different mechanism, different contract.

Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no
builtins in the implementation. The -O2 disassembly of
`q32_32_round_even` is `mov`/`xor`/`shr`/`test`/`setne`/`and`/
`or`/`add`/`ret` only, no conditional jumps (checked
programmatically in the test and independently by inspection).

Verified in `test_round_rne.c`:

- Exhaustive sweep: all 65,536 values with only the low 16
  fraction bits varying, integer part 0, differential-checked
  against an independent exact oracle.
- 11 directed rows: tie with integer even/odd, above half with
  sticky set, below half, carry into the integer part
  (0x00000000.FFFFFFFF -> 1), and the wraparound contract rows
  (0xFFFFFFFF.FFFFFFFF -> 0, 0xFFFFFFFF.80000000 -> 0,
  0xFFFFFFFF.80000001 -> 0, 0xFFFFFFFE.80000000 -> 0xFFFFFFFE).
- 1,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked.
- The oracle computes `2*frac` vs `2^32` in `unsigned __int128`,
  an arithmetic route that shares no bit identities with the
  implementation; on an exact half it rounds up iff the integer
  part is odd.
- 1,065,547 total checks, 0 mismatches. FNV-1a 64-bit checksum
  over the result stream: `258f283508816c19`, identical across
  `-O0`, `-O2`, and ASan+UBSan builds.
- Measured at -O2: `2.66` ns/value (best of 5 reps over a 1M-value
  buffer; the PRNG fill runs once before timing starts and is not
  inside the timed loop).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Renumber note: the backlog item was named lab/94-round-nearest-even,
but lab/94 is taken in this repo (lab/94-mulhi-signed), so the
module was built as lab/101, the lowest free lab number >= 100.

Run `make run` in this directory for the correctness builds, `make
bench` for throughput. The genuine build log and run output are in
`PROOF.md`.
