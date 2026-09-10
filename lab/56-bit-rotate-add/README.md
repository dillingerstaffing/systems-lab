# lab/56-bit-rotate-add

`rot_add.c` (`rot_add.h` declares it): two functions,
`uint64_t rotl64(uint64_t x, unsigned k)` and
`uint64_t rot_add64(uint64_t x, uint64_t y, unsigned k)`.

How it works: `rotl64` reduces k modulo 64, returns x unchanged when k
is 0, and otherwise computes `(x << k) | (x >> (64 - k))`. The k == 0
early return exists because shifting a 64-bit value by 64 is undefined
behavior in C; every shift that executes uses a count in 1..63.
`rot_add64` returns `rotl64(x, k) + y`; unsigned addition in C is
defined to wrap modulo 2^64, so overflow needs no check and cannot be
undefined behavior.

Verified by `test_rot_add.c` (fixed-seed splitmix64 PRNG, seed
`0x9E3779B97F4A7C15`, reset per rotation amount so all 64 amounts see
the same 1,000,000 values; fully reproducible):

- 10 directed cases with hand-computed answers: k = 0 is the identity,
  k = 64 and k = 128 reduce to the identity, k = 65 equals k = 1,
  `rotl64(0x0123456789ABCDEF, 1)` = `0x02468ACF13579BDE`,
  `rotl64(0x0123456789ABCDEF, 63)` = `0x8091A2B3C4D5E6F7`, a high-bit
  wrap (`rotl64(0x8000000000000000, 1)` = 1), and three `rot_add64`
  cases including a wrap to zero (`rot_add64(0xFFFFFFFFFFFFFFFF, 1, 0)`
  = 0).
- 64,000,000 differential cases (all 64 rotation amounts, 1,000,000
  random 64-bit values each), each checked three ways against an
  independent per-bit reference that moves every set bit to its
  rotated position individually and never uses the shift/OR rotate
  identity: the `rotl64` output equals the reference,
  `rotl64(ref_rotr(x, k), k)` equals x (rotate left after the
  reference rotate right is the identity), and `rot_add64(x, y, k)`
  equals the reference rotation plus the wrapping unsigned add.
  0 mismatches in all three checks.
- Identical FNV-1a checksum (`11571239451276328976`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2` (100,000,000 timed values, each drawn from the PRNG
and accumulated into a sink so the loop cannot be optimized away):
10.51 ns/value; the sink sum is 1018120567130385963. Honest caveats:
the figure includes the PRNG step, so it is the cost of one
generate-rotate-add case, not one bare `rot_add64` call, and
rerun-to-rerun machine variance is roughly +-1 ns.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
