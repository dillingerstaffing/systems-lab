# lab/106-sub-borrow-chain

`sub_borrow_chain(a_hi, a_lo, b_hi, b_lo, borrow_in, &diff_hi, &diff_lo)`
computes the 128-bit unsigned difference d = a - b - borrow_in from two
64-bit words plus a borrow-in, returning the final borrow-out. No
`__int128` anywhere in the implementation: each stage is one wrapping
64-bit addition and the borrow-out identity

```
t      = b + borrow_in            (wrapping)
borrow = (a < t) || (borrow_in && (t < b))
diff   = a - t                    (wrapping)
```

The exact borrow-out is 1 iff a < b + borrow_in as plain integers. The
`(t < b)` term is the wrap detector for the b == UINT64_MAX edge: then
t wraps to 0 while b + borrow_in == 2^64 exactly, which every a is
below, so a borrow always occurs and the term forces borrow = 1. The
borrow-out of the low stage (always 0 or 1) is the borrow-in of the
high stage, which uses the same identity. The derivation in PROOF.md is
the argument: nothing in this module says what it does, the construction
and the measurements say it.

`borrow_in` must be 0 or 1. All arithmetic is on unsigned 64-bit
values, so every wrap is well defined by C11. The `__int128` oracle in
the test file is independent: it computes the exact difference with
128-bit arithmetic and derives borrow-out from the inequality
a < b + borrow_in with its own wrap handling.

## What was measured

- 36 directed edge rows pinning the identity's corners: b_lo ==
  UINT64_MAX with borrow_in 1 (wrap detector fires for every a_lo),
  a_lo == 0, b_lo == 0, all-ones words both borrow values, borrow_in 1
  with a == b (exact difference -1), a == 0 with borrow_in 1, borrow
  chaining across the word boundary (2^64 - 1, -1 mod 2^128), b_hi ==
  UINT64_MAX with a low-stage borrow 1 (wrap detector on the high
  stage), and alternating bit patterns both borrow values.
  0 mismatches vs the `__int128` oracle.
- Exhaustive 16-bit (a_lo, b_lo) pairs x borrow_in 0/1 with high words
  0: 8,589,934,592 cases in 152.7 s wall, differential against the
  oracle: 0 mismatches. The low stage sees every 16-bit subtraction
  including every b_lo wrap case; the high stage pins the
  (0 - 0 - borrow_lo) row and the final borrow-out identity.
- 10,000,000 fixed-seed splitmix64 random 128-bit cases (seed
  `0x123456789ABCDEF0`, borrow_in alternating 0/1), differential
  against the oracle: 0 mismatches.
- Total: 8,599,934,628 cases, 0 mismatches.
- The FNV-1a checksum over per-case inputs and outputs,
  `0xfce86f5578b4ca16`, is identical across the `-O0`, `-O2`, and
  ASan+UBSan builds.
- Throughput at `-O2`: 4.55 ns/value (best of 5 over 1,000,000
  pre-generated cases, PRNG outside the timed region).

`-std=c11 -Wall -Wextra -Werror` clean; zero ASan/UBSan reports across
the full case set. Exact build logs and run output are in PROOF.md.
