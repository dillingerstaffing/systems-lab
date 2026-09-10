# PROOF.md - lab/72-bitmatrix-transpose

`bitmatrix_transpose64`: transpose of an 8x8 bit matrix packed in a
`uint64_t`, via three SWAR delta-swap stages at distances 7, 14, 28.

## Why the construction is exact

Write the 6-bit index of each bit position as r2 r1 r0 c2 c1 c0
(bits 5..0), where the bit sits at matrix row r = 4*r2+2*r1+r0 and
column c = 4*c2+2*c1+c0, linear index 8*r + c.  The transpose must map
index 8*r + c to 8*c + r, i.e. swap the two 3-bit halves of the index.

Swapping the halves factors into three swaps of single index bits:
(bit0, bit3), (bit1, bit4), (bit2, bit5).  Swapping index bits (a, b)
with a < b pairs positions at linear distance 2^b - 2^a: the lower
member has bit a = 1, bit b = 0, and flipping both changes the index
by 2^b - 2^a.  Hence the three stages are conditional swaps at
distances 7 = 8-1, 14 = 16-2, 28 = 32-4.

Each stage is the delta swap

    t = (x ^ (x >> d)) & m;  x ^= t ^ (t << d),

with m holding exactly the lower member of every pair (bit a = 1,
bit b = 0).  Adding d = 2^b - 2^a flips bit a (no carry into it, since
d has zeros below bit a), so i + d is never itself in the mask: the
pairs are disjoint and every position is either fixed (bits a, b
equal) or in exactly one pair.  Each stage is therefore a true
involution on positions, and the three compose to the index-bit swaps
above, i.e. to the transpose.  The stages use disjoint index-bit
pairs, so they commute.

All shifts are in range: the largest set bit of any mask plus its
stage distance stays below 64 (55+7=62, 46+14=60, 31+28=59), and all
arithmetic is unsigned, so there is no undefined behavior.

The masks were generated from the index-bit rule by an independent
Python script (not copied from any published constant) and the script
verified the simulated delta swaps against a naive per-bit reference
on 65,536 + 200,000 inputs with 0 mismatches before the C code was
written.

## Build log (verbatim)

```
$ make clean && make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_bitmatrix_transpose_o0 test_bitmatrix_transpose.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_bitmatrix_transpose_o2 test_bitmatrix_transpose.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_bitmatrix_transpose_asan test_bitmatrix_transpose.c
```

Zero warnings under `-Wall -Wextra -Werror`.

## Run output, build -O0 (verbatim)

```
bitmatrix_transpose64 differential test, build -O0
[1/4] exhaustive 16-bit inputs (65536 cases)
  done: cases=65536 mismatches=0 involution_failures=0
[2/4] 10M fixed-seed splitmix64 64-bit values
  done: cases=10065536 mismatches=0 involution_failures=0
[3/4] directed edge cases
  done: cases=10065622 mismatches=0 involution_failures=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 11.313 ns/value (88.391 M values/s)
  throughput pass 1: 10.891 ns/value (91.818 M values/s)
  throughput pass 2: 10.799 ns/value (92.602 M values/s)
  throughput pass 3: 11.799 ns/value (84.752 M values/s)
  throughput pass 4: 11.264 ns/value (88.779 M values/s)
  throughput best of 5: 10.799 ns/value (92.602 M values/s)
total verification cases: 10065622
total mismatches: 0
total involution failures: 0
FNV-1a checksum of all result words: 0xdb2cc52bd54d6a06
RESULT: PASS
```

## Run output, build -O2 (verbatim)

```
bitmatrix_transpose64 differential test, build -O2
[1/4] exhaustive 16-bit inputs (65536 cases)
  done: cases=65536 mismatches=0 involution_failures=0
[2/4] 10M fixed-seed splitmix64 64-bit values
  done: cases=10065536 mismatches=0 involution_failures=0
[3/4] directed edge cases
  done: cases=10065622 mismatches=0 involution_failures=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 3.135 ns/value (318.969 M values/s)
  throughput pass 1: 3.170 ns/value (315.414 M values/s)
  throughput pass 2: 3.288 ns/value (304.094 M values/s)
  throughput pass 3: 3.043 ns/value (328.574 M values/s)
  throughput pass 4: 3.031 ns/value (329.878 M values/s)
  throughput best of 5: 3.031 ns/value (329.878 M values/s)
total verification cases: 10065622
total mismatches: 0
total involution failures: 0
FNV-1a checksum of all result words: 0xdb2cc52bd54d6a06
RESULT: PASS
```

## Run output, build asan+ubsan (verbatim)

```
bitmatrix_transpose64 differential test, build asan+ubsan
[1/4] exhaustive 16-bit inputs (65536 cases)
  done: cases=65536 mismatches=0 involution_failures=0
[2/4] 10M fixed-seed splitmix64 64-bit values
  done: cases=10065536 mismatches=0 involution_failures=0
[3/4] directed edge cases
  done: cases=10065622 mismatches=0 involution_failures=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 4.677 ns/value (213.810 M values/s)
  throughput pass 1: 6.104 ns/value (163.815 M values/s)
  throughput pass 2: 4.178 ns/value (239.349 M values/s)
  throughput pass 3: 4.112 ns/value (243.199 M values/s)
  throughput pass 4: 4.132 ns/value (241.988 M values/s)
  throughput best of 5: 4.112 ns/value (243.199 M values/s)
total verification cases: 10065622
total mismatches: 0
total involution failures: 0
FNV-1a checksum of all result words: 0xdb2cc52bd54d6a06
RESULT: PASS
```

The FNV-1a checksum `0xdb2cc52bd54d6a06` is identical across all three
builds.  No AddressSanitizer or UBSan reports on the full suite.
