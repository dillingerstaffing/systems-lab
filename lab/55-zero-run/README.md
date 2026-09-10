# lab/55-zero-run

`zero_run.c` (`zero_run.h` declares it): longest run of consecutive
zero bits in a 64-bit word, `unsigned longest_zero_run(uint64_t x)`,
with `longest_zero_run(0)` defined as 64 and
`longest_zero_run(0xFFFFFFFFFFFFFFFF)` defined as 0.

How it works: `y = ~x` turns every zero-run of `x` into a one-run of
`y`. One round of `y &= y << 1` keeps bit i exactly when bits i and
i-1 were both set the round before, so each round deletes the lowest
set bit of every maximal one-run. After k rounds, bit i is set exactly
when bits i-k..i were all set in the original y, so y is nonzero after
k rounds exactly when the original y held a one-run of length k+1 or
more. The number of rounds until y reaches 0 is therefore the length
of the longest one-run of y, which is the longest zero-run of x. Only
shifts and AND are used; no count builtins and no bit-scan
instructions anywhere in the implementation. All arithmetic is
unsigned, so no undefined behavior is possible.

Verified by `test_zero_run.c` (fixed-seed splitmix64 PRNG, seed
`0x243F6A8885A308D3`, fully reproducible):

- Directed cases with hand-computed answers: `0` -> 64,
  all-ones -> 0, `0xAAAAAAAAAAAAAAAA` -> 1,
  `0x8000000000000001` -> 62, `0x00000000FFFFFFFF` -> 32.
- All 64 single-set-bit words (expected longest zero-run
  `max(k, 63-k)` for bit k) and all 64 single-clear-bit words
  (expected 1), covering every bit position.
- 1,065,669 differential checks, 0 mismatches: the directed cases
  plus the single-bit sweeps, an exhaustive sweep over all 2^16
  values 0..65535, and 1,000,000 full-range 64-bit random values,
  each compared against an independent naive bit-walking oracle.
- Identical FNV-1a checksum (`9935966997446426842`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.

Timing at `-O2` (100,000,000 timed values, each drawn from the PRNG
and accumulated into a sink so the loop cannot be optimized away):
31.47 ns/value; the sink sum of 535,639,027 means an average longest
run of 5.36 per timed value. Honest caveats: the figure includes the
PRNG step, so it is the cost of one generate-and-count case, not one
bare `longest_zero_run` call, and rerun-to-rerun machine variance is
roughly +-1 ns.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
