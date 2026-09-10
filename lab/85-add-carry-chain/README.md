# lab/85-add-carry-chain

`add128(a_hi, a_lo, b_hi, b_lo, carry_in, &sum_hi, &sum_lo, &carry_out)`
computes the 129-bit sum s = (a_hi:a_lo) + (b_hi:b_lo) + carry_in from
two 64-bit words plus a carry-in, returning the 128-bit sum and the
final carry-out. No 128-bit integer type anywhere in the implementation:
each stage is one wrapping 64-bit addition and the exact carry formula

```
sum   = a + b + carry_in          (wrapping)
carry = (sum < a) || (carry_in && (sum == a))
```

The exact carry is 1 iff a + b + carry_in >= 2^64 as integers. The
`(sum < a)` term is the carry-out identity for one addition; the second
term is the wrap detector for the b == UINT64_MAX edge: then
a + b = 2^64 - 1 (all ones), and a further carry_in of 1 wraps sum back
to exactly a (the a = 0 case gives sum = 0), where the bare identity
reports 0 while the true carry is 1. The carry-out of the low stage
(always 0 or 1) is the carry-in of the high stage, which uses the same
formula. The derivation in PROOF.md is the argument: nothing in this
module says what it does, the construction and the measurements say it.

`carry_in` must be 0 or 1. All arithmetic is on unsigned 64-bit values,
so every wrap is well defined by C11. The `__int128` oracle in the test
file is independent: it computes each stage exactly in 128-bit
arithmetic (every true stage sum stays below 2^65, so nothing wraps
inside the oracle) and never uses the identity under test.

## What was measured

- 24 directed edge rows pinning the formula's corners: a_lo = 0,
  b_lo = UINT64_MAX, carry_in 1 (bare identity says 0, true carry 1);
  c1 = 1 with lo_sum landing exactly on a_lo; s1 all-ones wrapped to 0
  by carry_in; all-ones words with both carry values (2^129 - 1);
  low carry wrapping the high word to 0; low carry into 5 + 7; the
  high-stage wrap detector (0 + UINT64_MAX + 1); sign-bit wraps;
  alternating bit patterns. 0 mismatches vs the `__int128` oracle.
- Exhaustive 16-bit (a_lo, b_lo) pairs x carry_in 0/1, high words zero:
  8,589,934,592 cases, 0 mismatches, carry-out checked on every case.
- 1,000,000 random full-width cases (splitmix64, fixed seed 20260910),
  carry_in 0/1 per case: 0 mismatches.
- Total differential coverage: 8,590,934,616 cases, 0 mismatches.
- FNV-1a checksum of the quick suite (edges + random):
  0x8214c49c79bb8760, identical across -O0, -O2, and ASan+UBSan builds.
  Zero warnings under -Wall -Wextra -Werror; zero sanitizer reports.
- Throughput at -O2: 12.526 ns per add128 call over 200,000,000 timed
  calls (dependency-chained; PROOF.md states exactly what the timed
  loop includes).
