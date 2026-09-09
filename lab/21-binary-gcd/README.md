# lab/21-binary-gcd

`bgcd.c` (`bgcd.h` declares it): Stein's binary GCD for 32-bit
unsigned integers, `uint32_t bgcd32(uint32_t u, uint32_t v)`, written
from scratch with no library gcd. It rests on three bit identities:

- both even: `gcd(u, v) = 2 * gcd(u/2, v/2)`, so the common factors
  of 2 are stripped up front with shifts;
- one even, one odd: the even operand's factor of 2 cannot divide the
  odd one, so it is shifted out without changing the gcd;
- both odd: their difference is even and shares every common divisor,
  so the larger is replaced by `v - u` (strictly smaller), and the
  loop repeats until `v` reaches 0.

`gcd(0, 0)` is defined as 0 so the function is total. All arithmetic
is unsigned and all shifts are 1-bit shifts on `uint32_t`, so no
undefined behavior is possible.

Verified by `test_bgcd.c` (fixed-seed xorshift64* PRNG, seed
`0x2545F4914F6CDD1D`, fully reproducible), differential against a
hand-written naive Euclid's algorithm with the same `gcd(0, 0) = 0`
definition:

- 2,048,576 total checks, 0 mismatches: exhaustive all-pairs over
  0..1023 (1,048,576 pairs, covering 0, 1, and powers of 2) plus
  1,000,000 full-range 32-bit pairs.
- Identical FNV-1a checksum (`11576187220487214191`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing per fully-checked case (see PROOF.md for the honest caveats):
at `-O2`, 528.21 ns/pair; at `-O0`, 682.45 ns/pair.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
