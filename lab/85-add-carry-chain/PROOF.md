# PROOF.md: lab/85-add-carry-chain

Date: 2026-09-10. Machine: x86_64, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine build logs and run outputs, pasted
unedited at the end.

`add128.c`/`add128.h` hold the implementation under test: `add128()`,
two 64-bit stages of the sum s = a + b + carry_in over 128-bit words,
using only wrapping 64-bit addition and the carry formula derived
below. The independent oracle in `test_add128.c` uses unsigned
`__int128` exact arithmetic in two stages (each true stage sum stays
below 2^65, so nothing wraps inside the oracle, and the oracle never
uses the identity under test). The implementation never uses a 128-bit
integer type (verified by grep over the sources, see the disasm log).

## The derivation (the carry formula)

Definitions: a, b are 64-bit words, cin in {0,1}.
sum = (a + b + cin) mod 2^64, the value C11 computes for the unsigned
expression a + b + cin. carry is 1 iff a + b + cin >= 2^64 as integers,
else 0.

Lemma (one addition, cin = 0): carry = 1 iff sum < a.
Proof. If a + b < 2^64, then sum = a + b >= a, so (sum < a) is false
and the carry is 0. If a + b >= 2^64, then sum = a + b - 2^64, and
sum < a iff b < 2^64, which always holds; so (sum < a) is true and the
carry is 1.

Claim: carry = (sum < a) | (cin & (sum == a)).
Proof. Write t = a + b, s1 = t mod 2^64, and c1 for the carry out of t
(c1 = (s1 < a) by the lemma). Then sum = (s1 + cin) mod 2^64.

Case c1 = 1 (t >= 2^64): the true carry is 1 whatever cin is. Here
s1 = t - 2^64 < a iff b < 2^64, always true, so s1 <= a - 1 (and
a >= 1: a = 0 would give t = b <= 2^64 - 1, contradicting c1 = 1).
With cin = 0, sum = s1 < a and the first term is true. With cin = 1,
sum = s1 + 1 unless s1 = 2^64 - 1, in which case sum = 0 < a (a >= 1)
and the first term is true; otherwise sum = s1 + 1 <= a, so either
sum < a (first term true) or sum = a (second term true). The formula
gives 1 in every sub-case.

Case c1 = 0 (t = a + b < 2^64): the true carry is 1 iff cin = 1 and
a + b = 2^64 - 1. With cin = 0, sum = a + b >= a, both terms are 0,
formula gives 0. With cin = 1 and a + b < 2^64 - 1, sum = a + b + 1
> a (b + 1 >= 1), both terms 0, formula gives 0. With cin = 1 and
a + b = 2^64 - 1, sum = 0: then (sum < a) iff a > 0. If a > 0 the
first term is true; if a = 0 (hence b = 2^64 - 1) then sum = a = 0
and the second term fires. The formula gives 1, matching the true
carry.

The a = 0, b = 2^64 - 1, cin = 1 sub-case is the edge the task calls
out: s1 = a + b = 2^64 - 1 (all ones), and adding cin wraps sum back
to 0 = a_lo. The bare identity (sum < a) evaluates 0 < 0 = false and
would report carry 0; the (cin & (sum == a)) term is exactly the
detector for this wrap through all-ones. (The nearby sub-case a = 5,
b = 2^64 - 1, cin = 1 has c1 = 1 and sum = 5 = a: first term false,
second term true.)

The high stage applies the same formula with c_lo as its carry-in.
c_lo is 0 or 1 (an OR of two 0/1 comparison results), so the claim
applies verbatim: with hi_sum = (a_hi + b_hi + c_lo) mod 2^64,
carry_out = (hi_sum < a_hi) | (c_lo & (hi_sum == a_hi)) is exact.

## What was run

- `quick` mode: 24 directed edge rows plus 1,000,000 random full-width
  cases (splitmix64, fixed seed 20260910, carry_in 0/1 per case).
  Run under -O0, -O2, and ASan+UBSan; the FNV-1a checksum over all
  inputs and outputs must be identical across the three builds.
- `full` mode (only -O2): quick plus the exhaustive 16-bit sweep,
  every (a_lo, b_lo) pair x carry_in 0/1 with high words zero,
  8,589,934,592 cases. sum_hi, sum_lo, and carry_out are compared
  against the oracle on every case.
- `bench` mode (-O2): 200,000,000 timed add128 calls. The timed loop
  per iteration performs one add128 call (separate translation unit,
  no LTO, so a genuine call with prologue/epilogue) plus about five
  ALU ops deriving the next inputs from the previous outputs and the
  loop counter; each call's carry_in is the previous call's
  carry_out, forming a dependency chain the compiler cannot elide.
  Final state is folded into a printed FNV-1a checksum so the loop
  stays live. The reported ns/value therefore includes call overhead
  and input derivation, not just the adder body.
- `disasm`: objdump of add128.o shows the compiled formula
  (branchless setb/sete), and grep confirms no 128-bit integer type
  token appears in add128.c/add128.h.
## Build logs (all three configurations; zero warnings)
```
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_add128 test_add128.c add128.c
exit=0
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_add128_o0 test_add128.c add128.c
exit=0
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_add128_asan test_add128.c add128.c
exit=0
```

## Run outputs: quick suite on -O0, -O2, ASan+UBSan (checksums identical)
```
mode=quick cases=1000024 mismatches=0 checksum=0x8214c49c79bb8760 elapsed=0.1s
mode=quick cases=1000024 mismatches=0 checksum=0x8214c49c79bb8760 elapsed=0.2s
mode=quick cases=1000024 mismatches=0 checksum=0x8214c49c79bb8760 elapsed=0.2s
```

## Run output: full suite at -O2 (exhaustive 16-bit + edges + random)
```
mode=full cases=8590934616 mismatches=0 checksum=0xd0a64bebc1998760 elapsed=184.3s
```

## Run output: throughput bench at -O2
```
bench: N=200000000 elapsed=2505.272 ms ns/value=12.526 checksum=0xd7d70cfc9e12e9e6
```

## Disassembly and 128-bit-type grep
```
--- add128 disassembly ---

add128.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <add128>:
   0:	f3 0f 1e fa          	endbr64
   4:	4c 01 c1             	add    %r8,%rcx
   7:	45 31 d2             	xor    %r10d,%r10d
   a:	4c 89 c0             	mov    %r8,%rax
   d:	48 01 f1             	add    %rsi,%rcx
  10:	41 0f 92 c2          	setb   %r10b
  14:	45 31 c0             	xor    %r8d,%r8d
  17:	48 39 ce             	cmp    %rcx,%rsi
  1a:	41 0f 94 c0          	sete   %r8b
  1e:	49 21 c0             	and    %rax,%r8
  21:	48 8d 04 17          	lea    (%rdi,%rdx,1),%rax
  25:	48 8b 54 24 08       	mov    0x8(%rsp),%rdx
  2a:	4d 09 d0             	or     %r10,%r8
  2d:	4c 01 c0             	add    %r8,%rax
  30:	48 89 0a             	mov    %rcx,(%rdx)
  33:	31 d2                	xor    %edx,%edx
  35:	48 39 c7             	cmp    %rax,%rdi
  38:	49 89 01             	mov    %rax,(%r9)
  3b:	0f 94 c2             	sete   %dl
  3e:	4c 21 c2             	and    %r8,%rdx
  41:	48 39 f8             	cmp    %rdi,%rax
  44:	0f 92 c0             	setb   %al
  47:	0f b6 c0             	movzbl %al,%eax
  4a:	48 09 d0             	or     %rdx,%rax
  4d:	48 8b 54 24 10       	mov    0x10(%rsp),%rdx
  52:	48 89 02             	mov    %rax,(%rdx)
  55:	c3                   	ret
--- __int128 token grep over add128.c add128.h (expect no output) ---
(no matches)
```
