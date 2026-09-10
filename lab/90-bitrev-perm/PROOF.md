# PROOF.md: lab/90-bitrev-perm

Date: 2026-09-10. Machine: x86_64, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build logs and run outputs.

`bitrev.c`/`bitrev.h` hold the implementation under test:
`bitrev8_perm(in, out)` writes `out[bitrev(i)] = in[i]` for i = 0..7,
where `bitrev` is the 3-bit index reversal computed only from the
swap identities (exchange bit 0 with bit 2, keep bit 1 fixed); no
lookup table, no library call doing the reversal.

The differential reference is `bitrev3_ref()` in `test_bitrev.c`, an
independently written explicit per-bit loop
(`r |= ((i >> b) & 1) << (2 - b)` for b = 0..2), a different code path
from the swap construction. The test driver runs 1,000,000 randomized
8-element arrays of distinct values (xorshift64* PRNG, seed
0x9E3779B97F4A7C15, values drawn without replacement) and applies
three checks per case:
1. element-wise equality between the implementation and the reference
2. bijection: with 8 distinct inputs, the output must contain each
   input value exactly once
3. involution: applying the permutation twice restores the original
   ordering

## Build log and run (-O2)

```
$ make clean && make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_bitrev test_bitrev.c bitrev.c
make exit=0
$ ./test_bitrev
randomized 8-element arrays of distinct values: 1000000 (seed 0x9E3779B97F4A7C15)
implementation vs per-bit-loop reference mismatches: 0
bijection check mismatches: 0
apply-twice identity mismatches: 0
fnv1a over all outputs: 0x43d798a1ea296710
timed sink (prevents dead-code elimination): 19139344518000000
throughput: 1.35 ns/value (best of 5; PRNG outside timed region)
RESULT: ALL TESTS PASSED
run exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## -O0 build and run

```
$ make opt0
=== BUILD -O0 ===
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_bitrev_o0 test_bitrev.c bitrev.c
./test_bitrev_o0
randomized 8-element arrays of distinct values: 1000000 (seed 0x9E3779B97F4A7C15)
implementation vs per-bit-loop reference mismatches: 0
bijection check mismatches: 0
apply-twice identity mismatches: 0
fnv1a over all outputs: 0x43d798a1ea296710
timed sink (prevents dead-code elimination): 19139344518000000
throughput: 3.80 ns/value (best of 5; PRNG outside timed region)
RESULT: ALL TESTS PASSED
opt0 exit=0
```

## ASan+UBSan build and run

```
$ make sanitize
=== BUILD ASan+UBSan ===
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_bitrev_asan test_bitrev.c bitrev.c
./test_bitrev_asan
randomized 8-element arrays of distinct values: 1000000 (seed 0x9E3779B97F4A7C15)
implementation vs per-bit-loop reference mismatches: 0
bijection check mismatches: 0
apply-twice identity mismatches: 0
fnv1a over all outputs: 0x43d798a1ea296710
timed sink (prevents dead-code elimination): 19139344518000000
throughput: 3.75 ns/value (best of 5; PRNG outside timed region)
RESULT: ALL TESTS PASSED
sanitize exit=0
```

The FNV-1a checksum over all 1,000,000 case outputs is identical
(0x43d798a1ea296710) across the -O2, -O0, and ASan+UBSan builds.

## Disassembly sanity check (-O2)

```
$ make disasm
gcc -std=c11 -O2 -Wall -Wextra -Werror -c -o bitrev.o bitrev.c
--- bitrev8_perm disassembly ---

bitrev.o:     file format elf64-x86-64

Disassembly of section .text:

0000000000000000 <bitrev8_perm>:
   0:	f3 0f 1e fa         	endbr64
   4:	31 d2               	xor    %edx,%edx
   6:	66 2e 0f 1f 84 00 00 	cs nopw 0x0(%rax,%rax,1)
   d:	00 00 00
  10:	89 d0               	mov    %edx,%eax
  12:	89 d1               	mov    %edx,%ecx
  14:	44 8b 04 97         	mov    (%rdi,%rdx,4),%r8d
  18:	c1 e9 02            	shr    $0x2,%ecx
  1b:	83 e0 02            	and    $0x2,%eax
  1e:	09 c8               	or     %ecx,%eax
  20:	8d 0c 95 00 00 00 00 	lea    0x0(,%rdx,4),%ecx
  27:	48 83 c2 01         	add    $0x1,%rdx
  2b:	83 e1 04            	and    $0x4,%ecx
  2e:	09 c8               	or     %ecx,%eax
  30:	44 89 04 86         	mov    %r8d,(%rsi,%rax,4)
  34:	48 83 fa 08         	cmp    $0x8,%rdx
  38:	75 d6               	jne    10 <bitrev8_perm+0x10>
  3a:	c3                  	ret
--- table / library reversal grep (expect no output) ---
```

Observation: the compiled body is only shift/mask/or/compare
instructions on the index, plus the two loads/stores. No memory
table load (no .rodata reference, no rip-relative table access),
no call to a library reversal routine; the reversal is computed
arithmetically per index. The grep for `rbit|reverse|table` in the
disassembly produced no output.
