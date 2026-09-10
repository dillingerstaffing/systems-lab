<!-- PROOF-HEADER
Checks: 4304971681
Mismatches: 0
Checksum: 0x309d79ae453ace68
Throughput: 2.744 ns/pair at -O2, best of 5
Verdict: PASS
-->
# PROOF.md - lab/70-mul-high

`mul_high64`: high 64 bits of the 128-bit product of two `uint64_t`,
from the 32-bit splitting identity
a = ah*2^32 + al, b = bh*2^32 + bl:

    high(a*b) = ah*bh + floor((al*bh + ah*bl + (al*bl >> 32)) / 2^32)

Why the construction is exact, in 64-bit unsigned arithmetic only:

- Each partial product is below 2^64 because each factor is below
  2^32, so `al*bl`, `al*bh`, `ah*bl`, `ah*bh` are exact in `uint64_t`.
- The middle sum `p1 + p2 + (p0 >> 32)` can reach 2^65, so it is
  formed with explicit wrap detection: for unsigned addition,
  `(x + y) < x` is 1 exactly when the addition wrapped past 2^64.
  With wrap bits c1 (from `p1 + p2`) and c2 (from adding `p0 >> 32`),
  `floor((p1 + p2 + (p0>>32)) / 2^32)` equals
  `(c1 + c2) * 2^32 + (((p1 + p2 + (p0>>32)) mod 2^64) >> 32)`,
  computed exactly in 64 bits.
- `p3 + q` cannot wrap: `p3 <= (2^32 - 1)^2 = 2^64 - 2^33 + 1` and
  `q <= 2^33 - 3`, so `p3 + q < 2^64`. The returned word is the true
  high word of the 128-bit product.
- The directed tests cover every reachable carry state: c1=1 via
  (`UINT64_MAX`, `UINT64_MAX`), c2=1 via
  (`0x80000000FFFFFFFF`, `0x80000001FFFFFFFF`) (both confirmed by
  instrumenting the intermediate values). c1=c2=1 is unreachable: it
  would require `(al*bl)>>32 >= 2^34 - 2`, but `(al*bl)>>32 < 2^32`.

Honest scope note: the exhaustive 16-bit sweep verifies the zero-high
path (for 16-bit inputs the high word is always 0, and the
implementation agrees on all 4.29B pairs). Coverage of the nonzero
cross terms comes from the 10M random 64-bit pairs and the directed
carry-state cases above.

## Build log (verbatim)

```
$ make clean && make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_mul_high_o0 test_mul_high.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_mul_high_o2 test_mul_high.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_mul_high_asan test_mul_high.c
```

Zero warnings under `-Wall -Wextra -Werror`. No `__int128` in
`mul_high.h` (verified with grep); it appears only in the test's exact
reference.

## Run output, build -O2 (verbatim)

```
mul_high64 differential test, build -O2
[1/4] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304971681 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 2.854 ns/pair
  throughput pass 1: 2.744 ns/pair
  throughput pass 2: 2.844 ns/pair
  throughput pass 3: 2.790 ns/pair
  throughput pass 4: 2.852 ns/pair
  throughput best of 5: 2.744 ns/pair
total verification cases: 4304971681
total mismatches: 0
FNV-1a checksum of all result words: 0x309d79ae453ace68
RESULT: PASS
```

## Run output, build asan+ubsan (verbatim)

```
mul_high64 differential test, build asan+ubsan
[1/4] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304971681 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 5.474 ns/pair
  throughput pass 1: 5.425 ns/pair
  throughput pass 2: 5.402 ns/pair
  throughput pass 3: 6.672 ns/pair
  throughput pass 4: 5.700 ns/pair
  throughput best of 5: 5.402 ns/pair
total verification cases: 4304971681
total mismatches: 0
FNV-1a checksum of all result words: 0x309d79ae453ace68
RESULT: PASS
```

No AddressSanitizer or UBSan findings on the full suite.

## Run output, build -O0 (verbatim)

```
mul_high64 differential test, build -O0
[1/4] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304971681 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 9.099 ns/pair
  throughput pass 1: 9.065 ns/pair
  throughput pass 2: 9.077 ns/pair
  throughput pass 3: 9.011 ns/pair
  throughput pass 4: 9.058 ns/pair
  throughput best of 5: 9.011 ns/pair
total verification cases: 4304971681
total mismatches: 0
FNV-1a checksum of all result words: 0x309d79ae453ace68
RESULT: PASS
```

The FNV-1a checksum over every result word is `0x309d79ae453ace68`
in all three builds.

## Disassembly check (verbatim)

`objdump -d` of the implementation compiled alone at `-O2`
(x86-64, GCC), via a non-inline wrapper so the function's own
codegen is visible:

```
/tmp/mh_wrap.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <mul_high64_wrap>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 89 f8            	mov    rax,rdi
   7:	89 f1               	mov    ecx,esi
   9:	89 ff               	mov    edi,edi
   b:	48 c1 ee 20         	shr    rsi,0x20
   f:	48 c1 e8 20         	shr    rax,0x20
  13:	48 89 fa            	mov    rdx,rdi
  16:	49 89 c0            	mov    r8,rax
  19:	48 0f af fe         	imul   rdi,rsi
  1d:	48 0f af c1         	imul   rax,rcx
  21:	48 0f af d1         	imul   rdx,rcx
  25:	4c 0f af c6         	imul   r8,rsi
  29:	48 01 c7            	add    rdi,rax
  2c:	0f 92 c0            	setb   al
  2f:	48 c1 ea 20         	shr    rdx,0x20
  33:	48 01 d7            	add    rdi,rdx
  36:	0f b6 c0            	movzx  eax,al
  39:	48 83 d0 00         	adc    rax,0x0
  3d:	48 c1 ef 20         	shr    rdi,0x20
  41:	48 c1 e0 20         	shl    rax,0x20
  45:	4c 01 c7            	add    rdi,r8
  48:	48 01 f8            	add    rax,rdi
  4b:	c3                  	ret
```

What this shows, honestly: the compiler emits the four partial
products as 64-bit `imul` in the two-operand form, which keeps only
the low 64 bits of the product. That is exact here because each
partial product is below 2^64 (both factors below 2^32, which the
compiler knows from the `mov edi,edi` zero-extension and the `shr`
splits). The middle-sum carries are folded with `add`/`setb`/`adc`,
matching the wrap-detection logic in the source. There is no
one-operand `mul` anywhere in this codegen, which is the instruction
that produces a 128-bit rdx:rax result, so the implementation does
not use a 128-bit multiply path; the `__int128` reference in the test
binary is the only code that does.

## Throughput methodology

The timed loop contains only `mul_high64` calls plus an xor into a
`volatile` sink (so the loop cannot be folded away). The 1M input
pairs are generated once with splitmix64 before timing starts, so
the PRNG step is not part of the measured time. Best of 5 passes is
reported; all 5 passes are listed above.
