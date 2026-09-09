# lab/23-floor-log2

`floor_log2.c` (`floor_log2.h` declares it): floor(log2(x)) for 64-bit
unsigned integers, `uint64_t floor_log2(uint64_t x)`, defined as the
index of the highest set bit. The implementation uses the shift/OR
bit-propagation identity: `x |= x >> 1; x |= x >> 2; x |= x >> 4;
x |= x >> 8; x |= x >> 16; x |= x >> 32;` fills every bit at or below
the highest set bit, so the result has exactly k + 1 bits set; a
from-scratch parallel (SWAR) popcount minus 1 yields k. No compiler
`clz` builtin or dedicated instruction is used anywhere in the
implementation.

Domain convention: x = 0 is excluded from the domain (log2(0) is
undefined). The implementation still returns a defined value: the
propagation leaves 0 unchanged, popcount(0) is 0, and 0 - 1 wraps to
`UINT64_MAX` in unsigned arithmetic. The test checks this convention
explicitly and runs the reference comparison only for nonzero inputs.

Verified by `test_floor_log2.c` (fixed-seed splitmix64, seed
`0x123456789ABCDEF0`, fully reproducible):

- 193 boundary cases: 0, 1, `2^k`, `2^k + 1` for k = 0..63,
  `2^k - 1` for k = 1..63, and `UINT64_MAX`; differential against
  `63 - __builtin_clzll` (nonzero inputs only; `__builtin_clzll` is
  undefined at 0, so the 0 case is checked against the documented
  `UINT64_MAX` convention instead).
- 10,000,000 random 64-bit values differential-tested against the
  same reference. 10,000,193 total checks, 0 mismatches.
- Identical FNV-1a checksum (`16451078516552681775`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2` (100,000,000 timed values, each drawn from the PRNG
and XORed into a sink so the loop cannot be optimized away):
7.7-7.9 ns/value. Honest caveats: the figure includes the PRNG step,
so it is the cost of one generate-and-count case, not one bare
`floor_log2` call, and rerun-to-rerun machine variance is roughly
+-0.2 ns. The 100M-value sink total was 11.

Disassembly check (gcc 13.3.0, `-O2`, x86_64): the compiler kept the
shift/OR propagation and SWAR popcount exactly as written (shr by
1/2/4/8/16/32 with `or`, then the popcount reduction); it did not
replace the sequence with a `bsr` or `lzcnt` instruction.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
