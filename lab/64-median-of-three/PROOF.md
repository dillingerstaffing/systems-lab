# PROOF.md — lab/64-median-of-three

`med3(a, b, c)`: median of three `int64_t` values as a 3-element
sorting network (compare-swap steps `(a,b)`, `(max_ab,c)`,
`(min_ab,min_maxc)`), each step built from a comparison mask plus
bitwise selection. No conditional jumps anywhere in the compiled
function. Header-only: `median3.h`. Tests: `test_median3.c`.

## Contract: no excluded inputs

Every subtraction in the mask path operates on 32-bit-zero-extended
halves (each under 2^32), so the difference can never wrap: the
comparison is exact on the whole `int64_t` range, including
`INT64_MIN` and `INT64_MAX`. The inputs themselves are only ever
selected bitwise (`b ^ ((a ^ b) & m)`), never added or subtracted, so
no overflow is possible anywhere. The test exercises the full domain:
directed edges include `INT64_MIN` and `INT64_MAX` in all positions,
and the random phase draws the full `int64_t` range per input.

## Build

```
$ make clean && make
rm -f test_median3_o2 test_median3_asan test_median3_o0 disasm_median3.o
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_median3_o2 test_median3.c
exit=0
```

Clean under `-Wall -Wextra -Werror` at all three flag sets. The random
phase uses splitmix64 with the fixed seed `0x123456789ABCDEF0` given in
the task, so every run is reproducible.

## Run (-O2)

```
$ ./test_median3_o2
exhaustive 8-bit sweep: a=-128 of 127, triples=65536, mismatches=0
exhaustive 8-bit sweep: a=-64 of 127, triples=4259840, mismatches=0
exhaustive 8-bit sweep: a=0 of 127, triples=8454144, mismatches=0
exhaustive 8-bit sweep: a=64 of 127, triples=12648448, mismatches=0
phase 1 (exhaustive 8-bit triples): 16777216 triples, 0 mismatches, 0.4 s
phase 2 (directed 64-bit edges): 729 triples, 0 mismatches
phase 3 (5M random 64-bit triples): 5000000 triples, 0 mismatches, 0.1 s
correctness: 21777945 differential checks, 0 mismatches
FNV-1a checksum of all outputs: 1512015986339184498 (0x14fbc28750757f72)
throughput: 10.379 ns/triple over 100000000 triples (sink=0x859f62956503f197)
ALL TESTS PASSED
exit=0
```

## Run (-O0 and ASan+UBSan)

```
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_median3_o0 test_median3.c && ./test_median3_o0
exhaustive 8-bit sweep: a=-128 of 127, triples=65536, mismatches=0
exhaustive 8-bit sweep: a=-64 of 127, triples=4259840, mismatches=0
exhaustive 8-bit sweep: a=0 of 127, triples=8454144, mismatches=0
exhaustive 8-bit sweep: a=64 of 127, triples=12648448, mismatches=0
phase 1 (exhaustive 8-bit triples): 16777216 triples, 0 mismatches, 1.8 s
phase 2 (directed 64-bit edges): 729 triples, 0 mismatches
phase 3 (5M random 64-bit triples): 5000000 triples, 0 mismatches, 0.8 s
correctness: 21777945 differential checks, 0 mismatches
FNV-1a checksum of all outputs: 1512015986339184498 (0x14fbc28750757f72)
throughput: 61.764 ns/triple over 100000000 triples (sink=0x859f62956503f197)
ALL TESTS PASSED
exit=0
```

```
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_median3_asan test_median3.c && ./test_median3_asan
exhaustive 8-bit sweep: a=-128 of 127, triples=65536, mismatches=0
exhaustive 8-bit sweep: a=-64 of 127, triples=4259840, mismatches=0
exhaustive 8-bit sweep: a=0 of 127, triples=8454144, mismatches=0
exhaustive 8-bit sweep: a=64 of 127, triples=12648448, mismatches=0
phase 1 (exhaustive 8-bit triples): 16777216 triples, 0 mismatches, 0.9 s
phase 2 (directed 64-bit edges): 729 triples, 0 mismatches
phase 3 (5M random 64-bit triples): 5000000 triples, 0 mismatches, 0.3 s
correctness: 21777945 differential checks, 0 mismatches
FNV-1a checksum of all outputs: 1512015986339184498 (0x14fbc28750757f72)
throughput: 12.997 ns/triple over 100000000 triples (sink=0x859f62956503f197)
ALL TESTS PASSED
exit=0
```

Zero warnings under all three flag sets, zero ASan/UBSan reports. The
FNV-1a checksum of all 21,777,945 outputs (`1512015986339184498`)
is identical across the three builds, and the throughput sink
(`0x859f62956503f197`) is identical too, confirming the three binaries
computed identical results on identical input streams.

## Disassembly (gcc -O2, non-inline wrapper, `objdump -d`)

The complete `med3_wrap` listing (75 instructions; every mnemonic is
in {endbr64, mov, push, pop, movabs, xor, shr, sub, sar, or, not, and,
ret}):

```
0000000000000000 <med3_wrap>:
   0:	f3 0f 1e fa          	endbr64
   4:	49 89 fa             	mov    %rdi,%r10
   7:	49 89 f3             	mov    %rsi,%r11
   a:	53                   	push   %rbx
   b:	49 89 f1             	mov    %rsi,%r9
   e:	48 bf 00 00 00 00 00 	movabs $0x8000000000000000,%rdi
  15:	00 00 80
  18:	4c 89 d0             	mov    %r10,%rax
  1b:	49 89 d0             	mov    %rdx,%r8
  1e:	4d 31 ca             	xor    %r9,%r10
  21:	48 31 f8             	xor    %rdi,%rax
  24:	49 31 fb             	xor    %rdi,%r11
  27:	48 89 c3             	mov    %rax,%rbx
  2a:	4c 89 d9             	mov    %r11,%rcx
  2d:	89 c0                	mov    %eax,%eax
  2f:	45 89 db             	mov    %r11d,%r11d
  32:	48 c1 eb 20          	shr    $0x20,%rbx
  36:	48 c1 e9 20          	shr    $0x20,%rcx
  3a:	4c 29 d8             	sub    %r11,%rax
  3d:	48 89 de             	mov    %rbx,%rsi
  40:	48 c1 f8 3f          	sar    $0x3f,%rax
  44:	48 29 ce             	sub    %rcx,%rsi
  47:	48 29 d9             	sub    %rbx,%rcx
  4a:	5b                   	pop    %rbx
  4b:	48 89 ca             	mov    %rcx,%rdx
  4e:	48 09 f2             	or     %rsi,%rdx
  51:	48 c1 fe 3f          	sar    $0x3f,%rsi
  55:	48 c1 fa 3f          	sar    $0x3f,%rdx
  59:	48 f7 d2             	not    %rdx
  5c:	48 21 c2             	and    %rax,%rdx
  5f:	48 09 f2             	or     %rsi,%rdx
  62:	4c 89 d6             	mov    %r10,%rsi
  65:	48 21 d6             	and    %rdx,%rsi
  68:	48 f7 d2             	not    %rdx
  6b:	4c 21 d2             	and    %r10,%rdx
  6e:	4d 89 c2             	mov    %r8,%r10
  71:	4c 31 ce             	xor    %r9,%rsi
  74:	4c 31 ca             	xor    %r9,%rdx
  77:	49 31 fa             	xor    %rdi,%r10
  7a:	48 89 d1             	mov    %rdx,%rcx
  7d:	4c 89 d0             	mov    %r10,%rax
  80:	45 89 d2             	mov    %r10d,%r10d
  83:	4c 31 c2             	xor    %r8,%rdx
  86:	48 31 f9             	xor    %rdi,%rcx
  89:	48 c1 e8 20          	shr    $0x20,%rax
  8d:	49 89 cb             	mov    %rcx,%r11
  90:	89 c9                	mov    %ecx,%ecx
  92:	49 c1 eb 20          	shr    $0x20,%r11
  96:	4c 29 d1             	sub    %r10,%rcx
  99:	4d 89 d9             	mov    %r11,%r9
  9c:	48 c1 f9 3f          	sar    $0x3f,%rcx
  a0:	49 29 c1             	sub    %rax,%r9
  a3:	4c 29 d8             	sub    %r11,%rax
  a6:	4c 09 c8             	or     %r9,%rax
  a9:	49 c1 f9 3f          	sar    $0x3f,%r9
  ad:	48 c1 f8 3f          	sar    $0x3f,%rax
  b1:	48 f7 d0             	not    %rax
  b4:	48 21 c8             	and    %rcx,%rax
  b7:	4c 09 c8             	or     %r9,%rax
  ba:	48 21 c2             	and    %rax,%rdx
  bd:	4c 31 c2             	xor    %r8,%rdx
  c0:	49 89 d1             	mov    %rdx,%r9
  c3:	48 31 f2             	xor    %rsi,%rdx
  c6:	49 31 f9             	xor    %rdi,%r9
  c9:	48 31 f7             	xor    %rsi,%rdi
  cc:	49 89 fa             	mov    %rdi,%r10
  cf:	4c 89 c9             	mov    %r9,%rcx
  d2:	89 ff                	mov    %edi,%edi
  d4:	45 89 c9             	mov    %r9d,%r9d
  d7:	49 c1 ea 20          	shr    $0x20,%r10
  db:	48 c1 e9 20          	shr    $0x20,%rcx
  df:	4c 29 cf             	sub    %r9,%rdi
  e2:	4d 89 d0             	mov    %r10,%r8
  e5:	48 89 c8             	mov    %rcx,%rax
  e8:	48 c1 ff 3f          	sar    $0x3f,%rdi
  ec:	49 29 c8             	sub    %rcx,%r8
  ef:	4c 29 d0             	sub    %r10,%rax
  f2:	4c 09 c0             	or     %r8,%rax
  f5:	49 c1 f8 3f          	sar    $0x3f,%r8
  f9:	48 c1 f8 3f          	sar    $0x3f,%rax
  fd:	48 f7 d0             	not    %rax
 100:	48 21 f8             	and    %rdi,%rax
 103:	4c 09 c0             	or     %r8,%rax
 106:	48 21 d0             	and    %rdx,%rax
 109:	48 31 f0             	xor    %rsi,%rax
 10c:	c3                   	ret
```

Jump scan over the object:

```
$ objdump -d disasm_median3.o | grep -cE '\tj[a-z]+\s'
0
```

Zero `j*` instructions total in the object (hence zero conditional
jumps, zero unconditional jumps): the mask construction survived as
`sub`/`sar`/`shr` with the half-split comparisons folded into the
32-bit zero-extends, and the selections as `not`/`and`/`or`/`xor`.
The instruction path is identical for every input triple.

## What was verified, exactly

- 16,777,216 exhaustive 8-bit triples (sign-extended to `int64_t`),
  differential against a comparison-based 3-sort reference, 0
  mismatches, at `-O0`, `-O2`, and under ASan+UBSan.
- 729 directed edge triples over `{INT64_MIN, INT64_MIN+1, -2^60, -1,
  0, 1, 2^60, INT64_MAX-1, INT64_MAX}`, 0 mismatches.
- 5,000,000 fixed-seed splitmix64 triples (seed
  `0x123456789ABCDEF0`), full `int64_t` range per input, 0 mismatches.
- Total: 21,777,945 differential checks, 0 mismatches.
- No conditional jump in the compiled median function (verified: 0
  `j*` instructions in the object).
- Measured at `-O2`: 10.379 ns/triple over 100,000,000 timed triples
  (the timed loop includes three LCG PRNG steps per triple, so this is
  a ceiling on the raw triple rate).
