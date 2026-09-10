# lab/73-mullo-split

`mullo64(a, b)` in `mullo_split.h`: the low 64 bits of the product of
two `uint64_t` values, computed from the 32-bit splitting identity
a = ah*2^32 + al, b = bh*2^32 + bl:

    low(a*b) = al*bl + ((al*bh + ah*bl) << 32) mod 2^64

The ah*bh term contributes nothing below bit 64, so it is not
computed. Each partial product fits in 64 bits (both factors are
below 2^32). The cross sum can reach 2^65, but only its value modulo
2^64 matters, because the following shift left by 32 discards the top
32 bits anyway, and C's unsigned arithmetic wraps modulo 2^64. No
128-bit type, no intrinsics, no builtins appear in the implementation;
the reference in the test is the native 64-bit product, which wraps
modulo 2^64 by the C standard.

Verified by `test_mullo.c`, differential against the native `a * b`
reference:

- Exhaustive: all 4,294,967,296 pairs of 16-bit inputs
  (0..65535 x 0..65535), 0 mismatches.
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0x123456789ABCDEF0`) 64-bit pairs, 0 mismatches.
- Directed: 4,385 cases, a 17x17 cross product (0, 1, 2,
  `UINT64_MAX`, half-word boundaries, alternating bit patterns) plus
  all 64x64 powers of two. The directed set forces the reachable
  carry states of the cross term: p1 + p2 wrapping modulo 2^64 via
  (`UINT64_MAX`, `UINT64_MAX`), and the top bit of the cross term
  being discarded by the shift in
  (`0x80000000FFFFFFFF`, `0x80000001FFFFFFFF`).
- 4,304,971,681 checks per build, 0 mismatches, in each of the `-O0`,
  `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every result word is identical across all
  three builds: `0x9c7c2747bf0beb25`.
- Disassembly of the implementation at `-O2` (x86-64, GCC): the three
  partial products survive as 64-bit `imul` in the two-operand form,
  followed by `add`/`shl`/`add` for the cross term. The compiler did
  NOT fold the split into a single 64-bit multiply; the 32-bit-split
  sequence survives in the emitted code.
- Throughput at `-O2`: best of 5 passes of 1M pairs, 1.645 ns/pair
  (passes: 1.645, 1.657, 3.668, 3.615, 3.723). The 1M pairs are
  generated once with splitmix64 before timing starts, so the PRNG is
  not part of the measured loop; results are xored into a volatile
  sink. `-O0`: 5.434 ns/pair; ASan+UBSan: 4.311 ns/pair.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full 4.3B-case suite.
