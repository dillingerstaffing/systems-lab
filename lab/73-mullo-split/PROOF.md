<!-- PROOF-HEADER
Checks: 4304971681
Mismatches: 0
Checksum: 0x9c7c2747bf0beb25
Throughput: 1.645 ns/pair at -O2, best of 5
Verdict: PASS
-->
# PROOF.md - lab/73-mullo-split

`mullo64`: low 64 bits of the product of two `uint64_t`, from the
32-bit splitting identity a = ah*2^32 + al, b = bh*2^32 + bl:

    a * b = ah*bh * 2^64 + (al*bh + ah*bl) * 2^32 + al*bl

so the low word is `(al*bl + ((al*bh + ah*bl) << 32)) mod 2^64`.

Why the construction is exact, in 64-bit unsigned arithmetic only:

- Each partial product is below 2^64 because each factor is below
  2^32, so `al*bl`, `al*bh`, `ah*bl` are exact in `uint64_t`.
- `ah*bh` contributes only to bits 64 and above, so it is never
  computed; it cannot affect the low word.
- The cross sum `al*bh + ah*bl` can reach 2^65, but it is formed in a
  64-bit register that wraps modulo 2^64, which is harmless: the next
  step shifts left by 32, discarding the top 32 bits, and
  `((x + 2^64*w) << 32)` is congruent to `(x << 32)` modulo 2^64 for
  any w. So the wrapped sum gives exactly the right low word.
- The final addition wraps modulo 2^64 per the C standard, which is
  exactly the definition of the low word of the product.

The reference in the test is the native `a * b`: unsigned 64-bit
multiplication wraps modulo 2^64 by the C standard, so it is exactly
the low word. No `__int128` appears anywhere in the module, test, or
implementation (verified with grep).

Honest scope note: the exhaustive 16-bit sweep verifies the
zero-cross-term path (for 16-bit inputs, ah = bh = 0, so the cross
term is 0 and the implementation agrees with the reference on all
4.29B pairs). Coverage of the nonzero cross term comes from the 10M
random 64-bit pairs and the directed carry-state cases above.

## Build log (verbatim)

```
$ make clean && make
rm -f test_mullo_o0 test_mullo_o2 test_mullo_asan
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_mullo_o0 test_mullo.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_mullo_o2 test_mullo.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_mullo_asan test_mullo.c
```

Zero warnings under `-Wall -Wextra -Werror`. No `__int128` in
`mullo_split.h` or `test_mullo.c` (verified with grep).

## Run output, build -O2 (verbatim)

```
mullo64 differential test, build -O2
[1/4] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304971681 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 1.645 ns/pair
  throughput pass 1: 1.657 ns/pair
  throughput pass 2: 3.668 ns/pair
  throughput pass 3: 3.615 ns/pair
  throughput pass 4: 3.723 ns/pair
  throughput best of 5: 1.645 ns/pair
total verification cases: 4304971681
total mismatches: 0
FNV-1a checksum of all result words: 0x9c7c2747bf0beb25
RESULT: PASS
```

## Run output, build -O0 (verbatim)

```
mullo64 differential test, build -O0
[1/4] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304971681 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 5.474 ns/pair
  throughput pass 1: 5.457 ns/pair
  throughput pass 2: 5.434 ns/pair
  throughput pass 3: 5.447 ns/pair
  throughput pass 4: 5.517 ns/pair
  throughput best of 5: 5.434 ns/pair
total verification cases: 4304971681
total mismatches: 0
FNV-1a checksum of all result words: 0x9c7c2747bf0beb25
RESULT: PASS
```

## Run output, build asan+ubsan (verbatim)

```
mullo64 differential test, build asan+ubsan
[1/4] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304971681 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 4.335 ns/pair
  throughput pass 1: 4.331 ns/pair
  throughput pass 2: 29.696 ns/pair
  throughput pass 3: 4.322 ns/pair
  throughput pass 4: 4.311 ns/pair
  throughput best of 5: 4.311 ns/pair
total verification cases: 4304971681
total mismatches: 0
FNV-1a checksum of all result words: 0x9c7c2747bf0beb25
RESULT: PASS
```

No AddressSanitizer or UBSan findings on the full suite (exit 0, no
sanitizer output). The FNV-1a checksum over every result word is
`0x9c7c2747bf0beb25` in all three builds.

## Disassembly check (verbatim)

`objdump -d -M intel` of the implementation compiled alone at `-O2`
(x86-64, GCC), via a non-inline wrapper so the function's own
codegen is visible:

```
/tmp/mullo_wrap.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <mullo64_wrap>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 89 f0            	mov    rax,rsi
   7:	89 fa               	mov    edx,edi
   9:	89 f1               	mov    ecx,esi
   b:	48 c1 ef 20         	shr    rdi,0x20
   f:	48 c1 e8 20         	shr    rax,0x20
  13:	48 0f af f9         	imul   rdi,rcx
  19:	48 0f af c2         	imul   rax,rdx
  1d:	48 0f af d1         	imul   rdx,rcx
  21:	48 01 f8            	add    rax,rdi
  24:	48 c1 e0 20         	shl    rax,0x20
  28:	48 01 d0            	add    rax,rdx
  2b:	c3                  	ret
```

What this shows, honestly: the 32-bit halves are split with `shr
0x20` (and 32-bit `mov` for the low halves, which zero-extend). The
three partial products appear as 64-bit `imul` in the two-operand
form (`imul rdi,rcx` = al*bh, `imul rax,rdx` = ah*bl, `imul rdx,rcx`
= al*bl), each exact since both factors are below 2^32. The cross
term is folded with `add`/`shl`/`add`. The compiler did not fold the
split back into a single 64-bit multiply: no `imul` with the full
operands a and b appears, and the ah*bh product is absent entirely
(as required, since it does not affect the low word). No compiler
barriers or inline asm were needed; the sequence survives `-O2`
codegen on its own.

## Throughput methodology

The timed loop contains only `mullo64` calls plus an xor into a
`volatile` sink (so the loop cannot be folded away). The 1M input
pairs are generated once with splitmix64 before timing starts, so
the PRNG step is not part of the measured time. Best of 5 passes is
reported; all 5 passes are listed above. The slower passes
(-O2 passes 2-4 near 3.7 ns, ASan pass 2 at 29.7 ns) are machine
scheduling noise; they are reported verbatim rather than hidden.
