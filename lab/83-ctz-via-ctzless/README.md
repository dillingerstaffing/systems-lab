# lab/83-ctz-via-ctzless

Count trailing zeros of a nonzero 64-bit word using only the identity
`ctz(x) = popcount((x ^ (x-1)) >> 1)`, where the popcount is a
hand-rolled SWAR bit count (no compiler intrinsics in the
implementation).

## What was measured

- 64 single-bit words (`1<<k`, k = 0..63): identity agrees with the
  shift-loop reference on all of them.
- All 65,535 nonzero 16-bit values, exhaustive: 0 mismatches.
- 1,000,000 fixed-seed splitmix64 64-bit values (seed
  `0x123456789ABCDEF0`, zeros redrawn so the domain stays nonzero):
  0 mismatches vs the shift-loop reference.
- The FNV-1a checksum over every case input and result is identical
  across the `-O0`, `-O2`, and ASan+UBSan builds.
- Disassembly at `-O2` (gcc 13.3.0, x86_64): neither the identity
  nor the shift-loop reference compiles to a native trailing-zero
  instruction. `ctz64_identity` stays a straight sequence of integer
  arithmetic (lea, xor, shifts, ands, sub, adds, one imul for the SWAR
  horizontal fold; no `tzcnt`, `bsf`, or `popcnt` anywhere). The
  shift-loop reference is inlined by gcc but left as a literal
  shift-and-test loop (`shr $1` / `test` / `jne`), not a `bsf`.
- Throughput of the identity at `-O2`: best-of-5 ns/value over the
  1,000,000 timed values, PRNG generation outside the timed region.

Contract: x must be nonzero. x = 0 is out of contract (pinned test row
records the observed value without guaranteeing it).

`-std=c11 -Wall -Wextra -Werror` clean; zero ASan/UBSan reports across
the full case set. Exact build logs and run output are in PROOF.md.
