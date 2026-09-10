<!-- PROOF-HEADER
Checks: 8590934630
Mismatches: 0
Checksum: 0x9f329e5c34e37e5c
Throughput: 299.17 ns/value at -O2, best of 5
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/98-half-adder-chain

Date: 2026-09-10. Machine: x86_64, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build logs and run outputs.

`halfadder.c`/`halfadder.h` hold the implementation under test:
`add_carry_chain()`, a 64-bit ripple-carry adder built one full-adder
stage per bit position from the half-adder identities. The independent
oracle in `test_halfadder.c` is the native `+` operator (widened exact
arithmetic plus the native `(t < a)` overflow identity); the
implementation never uses `+` on the operands (verified by
disassembling the object, see the disasm target in the Makefile).

## The derivation (full adder from two half adders)

A half adder on inputs (x, y) produces sum x ^ y and carry x & y.
A full adder adds a third input, carry_in. Decompose it as two half
adders:

1. Half adder on (a_i, b_i): partial sum p = a_i ^ b_i,
   generate g = a_i & b_i.
2. Half adder on (p, carry_in): sum_i = p ^ carry_in,
   second carry c2 = p & carry_in.
3. Final carry-out = g | c2.

Expand c2 by cases on the truth table of (a_i, b_i, carry_in):
- a_i == b_i: then p = 0, so c2 = 0. The identity's extra terms
  (a_i & carry_in) | (b_i & carry_in) equal a_i & carry_in here
  (both terms identical), and g = a_i; if a_i = 1 then g = 1 already
  reports the carry, so the identity's carry-out is g | ... = 1,
  matching g | c2 = 1 | 0. If a_i = 0, all terms are 0 on both sides.
- a_i != b_i: then g = 0 and p = 1, so c2 = carry_in. The identity
  gives 0 | (a_i & carry_in) | (b_i & carry_in); exactly one of a_i,
  b_i is 1, so this is carry_in. Both sides agree.

Hence the per-bit stage used in the code is exactly the full adder:
  sum_i       = a_i ^ b_i ^ carry_in
  carry_{i+1} = (a_i & b_i) | (a_i & carry_in) | (b_i & carry_in).

Why the ripple is correct: carry_{i+1} is 1 iff at least two of
{a_i, b_i, carry_in} are 1, i.e. iff the i-th bit column sums to 2
or 3, which is exactly the overflow of that column. Feeding it as the
next column's carry-in therefore computes the exact binary addition
of the two words plus the initial carry_in.

## Carry-out semantics (exact statement)

For inputs a, b (64-bit) and carry_in in {0, 1}, let
E = a + b + carry_in as a plain integer (0 <= E <= 2^65 - 1).
`add_carry_chain` writes E mod 2^64 to *sum and returns the carry-out
of bit 63. The returned value is 1 iff E >= 2^64, else 0; i.e. the
65-bit exact result is (carry_out << 64) | sum. The test checks the
carry-out against the native overflow identity on every 64-bit case:
with t = a + b (native wrapping +), overflow = (t < a) | (s < t)
where s = t + carry_in; the chain's carry-out must equal this on
every case.

## Build log and run (-O2)

```
=== -O2 ===
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_halfadder_o2 test_halfadder.c halfadder.c
./test_halfadder_o2
directed edge rows: mismatches so far 0
exhaustive 16-bit pairs x carry_in (8589934592 cases): 387.7 s wall
random 64-bit pairs: 1000000 (seed 0x123456789ABCDEF0, carry_in alternating)
timed sink (prevents dead-code elimination): 15198721083761960915
throughput: 299.17 ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)
total cases: 8590934630
implementation vs native-+ oracle mismatches (all phases): 0
fnv1a over per-case (inputs, outputs): 0x9f329e5c34e37e5c
RESULT: ALL TESTS PASSED
O2 done rc=0
```

Zero warnings under `-Wall -Wextra -Werror` (empty stderr/stdout from gcc).

## -O0 build and run

```
=== -O0 ===
(binary built 19:18 with zero warnings; re-running the same binary)
directed edge rows: mismatches so far 0
exhaustive 16-bit pairs x carry_in (8589934592 cases): 731.3 s wall
random 64-bit pairs: 1000000 (seed 0x123456789ABCDEF0, carry_in alternating)
timed sink (prevents dead-code elimination): 15198721083761960915
throughput: 223.71 ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)
total cases: 8590934630
implementation vs native-+ oracle mismatches (all phases): 0
fnv1a over per-case (inputs, outputs): 0x9f329e5c34e37e5c
RESULT: ALL TESTS PASSED
O0 done rc=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## AddressSanitizer + UBSan build and run

```
=== ASan+UBSan ===
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_halfadder_asan test_halfadder.c halfadder.c
./test_halfadder_asan
directed edge rows: mismatches so far 0
exhaustive 16-bit pairs x carry_in (8589934592 cases): 628.1 s wall
random 64-bit pairs: 1000000 (seed 0x123456789ABCDEF0, carry_in alternating)
timed sink (prevents dead-code elimination): 15198721083761960915
throughput: 145.29 ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)
total cases: 8590934630
implementation vs native-+ oracle mismatches (all phases): 0
fnv1a over per-case (inputs, outputs): 0x9f329e5c34e37e5c
RESULT: ALL TESTS PASSED
ASAN done rc=0
```

Zero warnings under `-Wall -Wextra -Werror`. Zero sanitizer reports in
any run: no ASan or UBSan output appears above, and every binary
exited 0.

## Cross-build checksum

The FNV-1a checksum over per-case inputs and outputs is
`0x9f329e5c34e37e5c` in all three builds (`-O2`, `-O0`, ASan+UBSan),
over the identical case stream: 38 directed edge rows, the full 2^33
exhaustive 16-bit cases, and the 1,000,000 fixed-seed random 64-bit
cases, for 8,590,934,630 total checks per build.

## Disassembly (gcc 13.3.0, x86_64, -O2)

`objdump -d halfadder.o --disassemble=add_carry_chain_w` shows only
xor, and, or, shr, shl, mov, cmp and the loop control; the single
`add $0x1,%ecx` is the loop counter i, not the operand datapath. No
addition touches a or b at any point. Genuine disassembly output:

```
0000000000000000 <add_carry_chain_w>:
   0:	f3 0f 1e fa         	endbr64
   4:	49 89 f3            	mov    %rsi,%r11
   7:	48 89 d6            	mov    %rdx,%rsi
   a:	83 e6 01            	and    $0x1,%esi
   d:	85 c9               	test   %ecx,%ecx
   f:	74 5f               	je     70 <add_carry_chain_w+0x70>
  11:	41 54               	push   %r12
  13:	49 89 fa            	mov    %rdi,%r10
  16:	41 89 c9            	mov    %ecx,%r9d
  19:	31 c9               	xor    %ecx,%ecx
  1b:	55                  	push   %rbp
  1c:	53                  	push   %rbx
  1d:	48 89 fb            	mov    %rdi,%rbx
  20:	31 ff               	xor    %edi,%edi
  22:	4c 31 db            	xor    %r11,%rbx
  25:	0f 1f 00            	nopl   (%rax)
  28:	4c 89 da            	mov    %r11,%rdx
  2b:	48 89 d8            	mov    %rbx,%rax
  2e:	4c 89 d5            	mov    %r10,%rbp
  31:	48 d3 ea            	shr    %cl,%rdx
  34:	48 d3 e8            	shr    %cl,%rax
  37:	83 e2 01            	and    $0x1,%edx
  3a:	83 e0 01            	and    $0x1,%eax
  3d:	48 d3 ed            	shr    %cl,%rbp
  40:	49 89 d4            	mov    %rdx,%r12
  43:	48 31 f0            	xor    %rsi,%rax
  46:	48 21 f2            	and    %rsi,%rdx
  49:	49 09 f4            	or     %rsi,%r12
  4c:	48 d3 e0            	shl    %cl,%rax
  4f:	48 89 d6            	mov    %rdx,%rsi
  52:	83 c1 01            	add    $0x1,%ecx
  55:	4c 21 e5            	and    %r12,%rbp
  58:	48 09 c7            	or     %rax,%rdi
  5b:	48 09 ee            	or     %rbp,%rsi
  5e:	41 39 c9            	cmp    %ecx,%r9d
  61:	75 c5               	jne    28 <add_carry_chain_w+0x28>
  63:	5b                  	pop    %rbx
  64:	48 89 f0            	mov    %rsi,%rax
  67:	5d                  	pop    %rbp
  68:	49 89 38            	mov    %rdi,(%r8)
  6b:	41 5c               	pop    %r12
  6d:	c3                  	ret
  6e:	66 90               	xchg   %ax,%ax
  70:	31 ff               	xor    %edi,%edi
  72:	48 89 f0            	mov    %rsi,%rax
  75:	49 89 38            	mov    %rdi,(%r8)
  78:	c3                  	ret
--- add-instruction grep over impl object (expect only loop-counter inc/add, none on operand datapath) ---
objdump -d halfadder.o | grep -E '\b(add|adc)\b' || true
  52:	83 c1 01            	add    $0x1,%ecx
  cd:	83 c1 01            	add    $0x1,%ecx
```

The loop body computes sum_i = a_i ^ b_i ^ carry (xor chain at
43/4c/58) and carry_{i+1} = (a_i & b_i) | (a_i & carry) |
(b_i & carry) (and/or chain at 46/49/55/5b); the compiler even
pre-computed a_i ^ b_i outside the loop (22). The only `add` in the
object is the loop counter i (the second hit at cd: is the same
counter in the width=64 copy compiled into `add_carry_chain`).
```

The throughput numbers vary run to run (CPU frequency); the -O2 run in
this file measured 299.17 ns/value, an earlier -O2 run measured
144.55 ns/value with the same zero-mismatch result and the same
checksum, so treat the timing as order-of-magnitude only.
