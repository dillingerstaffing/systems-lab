# lab/70-mul-high

`mul_high64(a, b)` in `mul_high.h`: the high 64 bits of the 128-bit
product of two `uint64_t` values, computed from the 32-bit splitting
identity a = ah*2^32 + al, b = bh*2^32 + bl:

    high(a*b) = ah*bh + floor((al*bh + ah*bl + (al*bl >> 32)) / 2^32)

Each partial product fits in 64 bits (both factors are below 2^32).
The middle sum can reach 2^65, so it is formed in a 64-bit register
with explicit wrap detection (`(x + y) < x` is 1 exactly when the
addition wrapped). No 128-bit type, no intrinsics, no builtins appear
in the implementation; `unsigned __int128` is used only in the test's
exact reference.

Verified by `test_mul_high.c`, differential against the `__int128`
reference:

- Exhaustive: all 4,294,967,296 pairs of 16-bit inputs
  (0..65535 x 0..65535), 0 mismatches.
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0x123456789ABCDEF0`) 64-bit pairs, 0 mismatches.
- Directed: 4,385 cases, a 17x17 cross product (0, 1, 2,
  `UINT64_MAX`, half-word boundaries, alternating bit patterns) plus
  all 64x64 powers of two. The directed set covers every reachable
  carry state of the middle sum: c1=1 via
  (`UINT64_MAX`, `UINT64_MAX`), c2=1 via
  (`0x80000000FFFFFFFF`, `0x80000001FFFFFFFF`); c1=c2=1 is impossible
  since it would require `(al*bl)>>32 >= 2^34 - 2`.
- 4,304,971,681 checks per build, 0 mismatches, in each of the `-O0`,
  `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every result word is identical across all
  three builds: `0x309d79ae453ace68`.
- Disassembly of the implementation at `-O2` (x86-64, GCC): four
  64-bit `imul` in the two-operand low-half-only form (exact here,
  since each partial product is below 2^64), with the carries folded
  through `add`/`setb`/`adc`. No one-operand `mul` (the instruction
  that produces a 128-bit rdx:rax result) appears in the
  implementation's codegen.
- Throughput at `-O2`: best of 5 passes of 1M pairs, 2.744 ns/pair
  (passes: 2.854, 2.744, 2.844, 2.790, 2.852). The 1M pairs are
  generated once with splitmix64 before timing starts, so the PRNG is
  not part of the measured loop; results are xored into a volatile
  sink. `-O0`: 9.011 ns/pair; ASan+UBSan: 5.402 ns/pair.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full 4.3B-case suite.
