# lab/72-bitmatrix-transpose

`bitmatrix_transpose64(x)` in `bitmatrix_transpose.h`: transpose of an
8x8 bit matrix packed in a `uint64_t`. Bit (r, c) at linear index
8*r + c moves to index 8*c + r.

The 6-bit index is r2 r1 r0 c2 c1 c0 (bits 5..0). The transpose swaps
the two 3-bit halves, which factors into three swaps of single index
bits: (bit0, bit3), (bit1, bit4), (bit2, bit5). Swapping index bits
(a, b) pairs positions at linear distance 2^b - 2^a, so the three
stages are conditional swaps at distances 7 = 8-1, 14 = 16-2, and
28 = 32-4. Each stage is a delta swap,

    t = (x ^ (x >> d)) & m;  x ^= t ^ (t << d),

with the mask holding exactly the lower member of every pair
(positions with bit a = 1, bit b = 0):

    d=7:  0x00AA00AA00AA00AA
    d=14: 0x0000CCCC0000CCCC
    d=28: 0x00000000F0F0F0F0

No tables, no builtins, no library code: only unsigned 64-bit shifts,
xors, and ands. The stages touch disjoint index-bit pairs, so they
commute and any order gives the same result.

Verified by `test_bitmatrix_transpose.c`, differential against an
independent naive per-bit-loop reference:

- Exhaustive: all 65,536 16-bit inputs, 0 mismatches.
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0x123456789ABCDEF0`) 64-bit values, 0 mismatches.
- Directed: 86 cases (zero, all-ones, all 64 single-bit positions,
  alternating row/column/checkerboard patterns, all 8 single rows and
  all 8 single columns).
- Involution: transpose(transpose(x)) == x checked on every one of
  the 10,065,622 cases above, 0 failures.
- 10,065,622 checks per build, 0 mismatches and 0 involution
  failures, in each of the `-O0`, `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every result word is identical across all
  three builds: `0xdb2cc52bd54d6a06`.
- The masks were also derived independently in Python from the
  index-bit rule and cross-checked against the naive reference on
  65,536 + 200,000 inputs before the C code was written: 0
  mismatches.
- Throughput at `-O2`: best of 5 passes of 1M values, 3.031 ns/value
  (329.878 M values/s); passes: 3.135, 3.170, 3.288, 3.043, 3.031.
  The 1M values are generated once with splitmix64 before timing
  starts, so the PRNG is not part of the measured loop; results are
  xored into a volatile sink. `-O0`: 10.799 ns/value; ASan+UBSan:
  4.112 ns/value.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full 10M-case suite.
