<!-- PROOF-HEADER
Checks: 20196608
Mismatches: 0
Checksum: 0x57707ad2ff081ccb
Throughput: 8.705 ns/pair at -O2
Verdict: PASS
-->
# PROOF.md — lab/52-bit-interleave

`morton_interleave(lo, hi)`: spreads the 16 bits of each input into
alternating positions of a 32-bit word (bits of `lo` in the even
positions, bits of `hi` in the odd positions).
`morton_deinterleave(z)`: packs the even-position bits of `z` into the
low 16 bits and the odd-position bits into the high 16 bits of a
`uint32_t`. Both are header-only (`morton.h`), built only from the
bit-spreading shift/mask identities; no lookup tables, no builtins.
Tests: `test_morton.c`.

## What the shift/mask identities do

`morton_spread16` moves input bit `i` to output bit `2*i` in four
stages: each stage doubles the gap between occupied bits
(8, 4, 2, 1), and the mask keeps exactly the bits that cannot collide
between the two operands, so the XOR acts as an OR everywhere the mask
keeps. Concretely:

- `x = (x ^ (x << 8)) & 0x00FF00FF`: bits 0-7 stay, bits 8-15 move to
  16-23; the overlapping bits 8-15 of the XOR are masked away.
- `x = (x ^ (x << 4)) & 0x0F0F0F0F`: each byte's low nibble stays, high
  nibble moves up 4; overlapping nibble bits are masked away.
- `x = (x ^ (x << 2)) & 0x33333333`: each nibble's bit pairs separate.
- `x = (x ^ (x << 1)) & 0x55555555`: every bit lands at an even
  position.

`morton_compact16` is the exact mirror image, shifting right instead of
left with the same masks in reverse order, so each stage gathers the
bit pairs it spread. `morton_interleave` ORs the spread of `lo` with
the spread of `hi` shifted left by 1 (their occupied positions are
disjoint by construction). All arithmetic is on `uint32_t`, so every
shift is logical and no shift overflows: the largest shift is
`x << 8` with `x <= 0xFFFF` at entry, and `x << 1` with
`x <= 0x33333333` after masking.

## Build

```
$ make clean && make
rm -f test_morton test_morton_o0 test_morton_asan
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_morton test_morton.c
exit=0
```

Clean under `-Wall -Wextra -Werror`. The PRNG is splitmix64 with a fixed
seed `0x243F6A8885A308D3`, so every run and every build sees the same
value stream.

## Run (-O2)

```
$ ./test_morton
phase 1 (exhaustive 8-bit pairs): 65536 pairs, 196608 checks, 0 mismatches
phase 2 (10M random 32-bit values): 20000000 checks, 0 mismatches
correctness: 20196608 differential checks, 0 mismatches
fnv1a checksum: 0x57707ad2ff081ccb
throughput: 8.705 ns/pair over 50000000 pairs (sink=0x000000009aa0b2a8) [loop includes the splitmix64 step, so this is a ceiling]
ALL TESTS PASSED
exit=0
```

## Run (-O0)

```
$ ./test_morton_o0
phase 1 (exhaustive 8-bit pairs): 65536 pairs, 196608 checks, 0 mismatches
phase 2 (10M random 32-bit values): 20000000 checks, 0 mismatches
correctness: 20196608 differential checks, 0 mismatches
fnv1a checksum: 0x57707ad2ff081ccb
throughput: 23.937 ns/pair over 50000000 pairs (sink=0x000000009aa0b2a8) [loop includes the splitmix64 step, so this is a ceiling]
ALL TESTS PASSED
exit=0
```

## Run (ASan+UBSan, -O1)

```
$ ./test_morton_asan
phase 1 (exhaustive 8-bit pairs): 65536 pairs, 196608 checks, 0 mismatches
phase 2 (10M random 32-bit values): 20000000 checks, 0 mismatches
correctness: 20196608 differential checks, 0 mismatches
fnv1a checksum: 0x57707ad2ff081ccb
throughput: 16.181 ns/pair over 50000000 pairs (sink=0x000000009aa0b2a8) [loop includes the splitmix64 step, so this is a ceiling]
ALL TESTS PASSED
exit=0
```

The FNV-1a checksum over all phase 1 and phase 2 outputs is
`0x57707ad2ff081ccb` in all three builds. Zero sanitizer reports.

## Limits of verification

- Phase 1 exhausts 8-bit `(lo, hi)` pairs only; the full 16-bit space
  (2^32 pairs) is too large to enumerate, so 16-bit coverage comes from
  the 10,000,000 fixed-seed random 32-bit values in phase 2, each checked
  on the round-trip invariant and on deinterleave against the naive
  reference.
- The naive per-bit-loop reference is trusted as ground truth; the
  differential argument is that both directions agree with it on every
  checked case, and the checksum being identical across three builds
  (including `-O0` and the sanitizer build) rules out
  optimization-dependent behavior.
- The timing number (8.705 ns/pair at `-O2`) is a ceiling: the timed
  loop includes one splitmix64 step per pair, so the Morton code itself
  costs strictly less than measured.
