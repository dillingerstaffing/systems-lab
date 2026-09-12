# lab/119-onesc-add

`onesc_add.c` (`onesc_add.h` declares it): 16-bit one's-complement
addition, `uint16_t onesc_add(uint16_t a, uint16_t b)`, where the
wraparound carry is folded back in.

How it works: the sum is taken in 16-bit wraparound arithmetic, and
the single lost carry bit is recovered from the wraparound identity
itself. `s = (uint16_t)(a + b)` can only be less than `a` when the
addition wrapped past 0xFFFF, so the comparison `(s < a)` is exactly
the 0/1 carry, no wider accumulator or checksum loop involved. The
carry is then added back into the low 16 bits. This is the isolated
adder; it has no checksum application in it. The all-ones identity
is pinned: `onesc_add(0xFFFF, 0xFFFF) == 0xFFFF`, not zero, because
the end-around carry turns the zero-sum into the all-ones value.

Verified by `test_onesc_add.c` (fixed-seed splitmix64 PRNG, seed
`0x123456789ABCDEF0`, fully reproducible):

- Edge pins: 0xFFFF + 0xFFFF == 0xFFFF, 0 + 0 == 0,
  0xFFFF + 0 == 0xFFFF, 0x8000 + 0x8000 == 0x0001, all agreeing
  with the loop-fold oracle.
- 10,065,536 differential checks, 0 mismatches: 65,536 directed
  pairs (a sweeps 0..65535 with b = 0xFFFF, forcing the carry on
  every nonzero case) plus 10,000,000 random 16-bit pairs, all
  compared against a structurally different loop-fold reference
  (32-bit sum, `while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16)`).
- Identical FNV-1a checksum (`12261447224700235540`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2` (100,000,000 timed pairs, best of 5 rounds, each
pair drawn from the PRNG and accumulated into a sink so the loop
cannot be optimized away): 3.14 ns/pair. Honest caveat: the figure
includes the PRNG step, so it is the cost of one generate-and-add
case, not one bare `onesc_add` call.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
