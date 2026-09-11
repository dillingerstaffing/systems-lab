<!-- PROOF-HEADER
Checks: 17777216
Mismatches: 0
Checksum: 7c8e5c077c4d942c
Throughput: 47.232 ns/value at -O2
Environment: Host
Verdict: PASS
-->
# PROOF: lab/128-mul-lo32-nomul

`mullo32(a, b)` returns the low 32 bits of the 64-bit product `a * b`,
computed with no multiply operator anywhere in the implementation.

## What was built

`mul32.h`, `mul32.c`, `test_mullo32.c`, `bench_mullo32.c`, `Makefile`,
`README.md`, this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`,
zero warnings, no intrinsics, no builtins, no library calls in the
implementation. The oracle in the test is the native C expression
`(uint32_t)(a * b)`, a structurally different computation from the
implementation's shift-add schoolbook expansion, so agreement pins the
identity rather than a shared bug.

## The construction

Write `A = a mod 2^32 = a0 + a1 * 2^16` and `B = b mod 2^32 = b0 + b1 * 2^16`
with 16-bit halves. The schoolbook expansion is

    A * B = a0*b0 + (a0*b1 + a1*b0) * 2^16 + a1*b1 * 2^32.

Modulo 2^32 the `a1*b1` term vanishes, and of the middle term only its
low 16 bits survive the 2^16 scaling. Every partial product touching
bits 32..63 of either input carries a factor of 2^32 or more and
vanishes the same way, which is why the low 32 bits of the product
depend only on the low 32 bits of the operands. Each 16x16 partial
product is exact in 32 bits and is built by shift-add (`mul16` in
`mul32.c`): for each set bit `i` of one half, add the other half
shifted left by `i`.

A `grep` for the `*` character over `mul32.c` and `mul32.h` shows hits
only in comment delimiters and in prose inside comments; no multiply
operator appears in code. The programmatic check below is stronger:
it scans the actual `-O2` object code.

## Build log (verbatim)

```
$ make test_mullo32_o2 test_mullo32_o0 test_mullo32_asan
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_mullo32_o2 test_mullo32.c mul32.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_mullo32_o0 test_mullo32.c mul32.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_mullo32_asan test_mullo32.c mul32.c
```

Compiler: `gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`, host x86-64.

## Disassembly check (verbatim, `make disasm`)

`make disasm` compiles `mul32.c` alone at `-O2`, dumps the object, then
counts objdump lines whose mnemonic is `mul`/`imul` (with any AT&T size
suffix) and fails the target unless the count is zero. gcc 13.3.0
compiled the three shift-add loops branchless (`bt` + `cmovb`) and
inlined them into `mullo32`. Result: 0 multiply instructions.

```
$ make disasm
gcc -std=c11 -Wall -Wextra -Werror -O2 -c mul32.c -o mul32.o
objdump -d mul32.o

mul32.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <mullo32>:
   0:	f3 0f 1e fa          	endbr64
   4:	41 89 f8             	mov    %edi,%r8d
   7:	41 89 f2             	mov    %esi,%r10d
   a:	0f b7 d7             	movzwl %di,%edx
   d:	31 c0                	xor    %eax,%eax
   f:	41 c1 e8 10          	shr    $0x10,%r8d
  13:	0f b7 fe             	movzwl %si,%edi
  16:	41 c1 ea 10          	shr    $0x10,%r10d
  1a:	89 d1                	mov    %edx,%ecx
  1c:	31 f6                	xor    %esi,%esi
  1e:	66 90                	xchg   %ax,%ax
  20:	0f a3 c7             	bt     %eax,%edi
  23:	44 8d 0c 0e          	lea    (%rsi,%rcx,1),%r9d
  27:	41 0f 42 f1          	cmovb  %r9d,%esi
  2b:	83 c0 01             	add    $0x1,%eax
  2e:	01 c9                	add    %ecx,%ecx
  30:	83 f8 10             	cmp    $0x10,%eax
  33:	75 eb                	jne    20 <mullo32+0x20>
  35:	31 c9                	xor    %ecx,%ecx
  37:	31 c0                	xor    %eax,%eax
  39:	0f 1f 80 00 00 00 00 	nopl   0x0(%rax)
  40:	41 0f a3 c2          	bt     %eax,%r10d
  44:	44 8d 0c 11          	lea    (%rcx,%rdx,1),%r9d
  48:	41 0f 42 c9          	cmovb  %r9d,%ecx
  4c:	83 c0 01             	add    $0x1,%eax
  4f:	01 d2                	add    %edx,%edx
  51:	83 f8 10             	cmp    $0x10,%eax
  54:	75 ea                	jne    40 <mullo32+0x40>
  56:	31 c0                	xor    %eax,%eax
  58:	31 d2                	xor    %edx,%edx
  5a:	66 0f 1f 44 00 00    	nopw   0x0(%rax,%rax,1)
  60:	0f a3 d7             	bt     %edx,%edi
  63:	46 8d 0c 00          	lea    (%rax,%r8,1),%r9d
  67:	41 0f 42 c1          	cmovb  %r9d,%eax
  6b:	83 c2 01             	add    $0x1,%edx
  6e:	45 01 c0             	add    %r8d,%r8d
  71:	83 fa 10             	cmp    $0x10,%edx
  74:	75 ea                	jne    60 <mullo32+0x60>
  76:	c1 e1 10             	shl    $0x10,%ecx
  79:	c1 e0 10             	shl    $0x10,%eax
  7c:	01 f1                	add    %esi,%ecx
  7e:	01 c8                	add    %ecx,%eax
  80:	c3                   	ret
--- multiply scan (mnemonic mul/imul) ---
multiply instructions found: 0
```

## Differential test runs (verbatim)

Phase 1: `a` exhaustive over all 16,777,216 24-bit values, `b` a
deterministic avalanche (one splitmix64 finalizer pass) of `a`.
Phase 2: 1,000,000 pairs of full 64-bit values from splitmix64 with the
fixed seed `0x123456789ABCDEF0`. Oracle: native `(uint32_t)(a * b)`.
Checksum: 64-bit FNV-1a over the 4 bytes (least significant first) of
every implementation output.

```
$ ./test_mullo32_o2
phase1 (exhaustive 24-bit a): checks=16777216 mismatches=0
phase2 (1M random 64-bit pairs): checks=17777216 mismatches=0
total checks=17777216 mismatches=0 checksum=7c8e5c077c4d942c
VERDICT: PASS
$ ./test_mullo32_o0
phase1 (exhaustive 24-bit a): checks=16777216 mismatches=0
phase2 (1M random 64-bit pairs): checks=17777216 mismatches=0
total checks=17777216 mismatches=0 checksum=7c8e5c077c4d942c
VERDICT: PASS
$ ./test_mullo32_asan
phase1 (exhaustive 24-bit a): checks=16777216 mismatches=0
phase2 (1M random 64-bit pairs): checks=17777216 mismatches=0
total checks=17777216 mismatches=0 checksum=7c8e5c077c4d942c
VERDICT: PASS
```

The checksum is byte-identical across `-O0`, `-O2`, and ASan+UBSan;
the sanitizer build printed no ASan/UBSan reports.

## Bench (verbatim, `make bench`)

```
$ make bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -o bench_mullo32 bench_mullo32.c mul32.c
./bench_mullo32
round 0: 47.232 ns/value (sink=009899cbdd4e3a00)
round 1: 63.760 ns/value (sink=009899cbdd4e3a00)
round 2: 54.630 ns/value (sink=009899cbdd4e3a00)
round 3: 56.509 ns/value (sink=009899cbdd4e3a00)
round 4: 51.534 ns/value (sink=009899cbdd4e3a00)
best of 5: 47.232 ns/value
```

What the timed loop includes, honestly: per value, one LCG step to
produce the next 64-bit input, one real call to `mullo32` (the
implementation lives in a separate translation unit and the bench is
built without LTO, so the call is not inlined away), and one 64-bit
accumulate into a sink that is printed so the calls cannot be dead-code
eliminated. The reported ns/value is total wall time divided by
20,000,000 values, so it covers the LCG step and the accumulate
alongside the call. Identical sinks across rounds confirm the same
work ran each round.

## Environment

Host: this VM, x86-64. `gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0`.
