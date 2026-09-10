<!-- PROOF-HEADER
Mismatches: 0
Checksum: 0xdd36dc676b59b047
Throughput: 3.00 ns/value at -O2, best of 5
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/83-ctz-via-ctzless

Date: 2026-09-10. Machine: x86_64, 2 cores, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build logs and run outputs.

`ctz.c`/`ctz.h` hold the implementation under test: `ctz64_identity()`,
built only from the identity `ctz(x) = popcount((x ^ (x-1)) >> 1)` with
a hand-rolled SWAR popcount. No `__builtin_ctz`, no
`__builtin_popcountll`, no tables. The differential reference is the
independent shift loop `ctz64_ref()` in `test_ctz.c`.

## Build log and run (-O2)

```
$ make clean && make
rm -f test_ctz test_ctz_asan test_ctz_o0 *.o
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_ctz test_ctz.c ctz.c
make exit=0

$ ./test_ctz
ok   single-bit words k=0..63: mismatches 0
ok   exhaustive 16-bit (65535 values): mismatches 0
random 64-bit cases: 1000000 (zeros redrawn: 0)
identity vs shift-loop mismatches (all phases): 0
fnv64 over per-case (input, result): 0xdd36dc676b59b047
contract pin: ctz64_identity(0) = 63 (x=0 is out of contract; observed only, no guarantee)
timed sink (prevents dead-code elimination): 5006300
throughput: 3.00 ns/value (best of 5; PRNG not in timed region)
RESULT: ALL TESTS PASSED
run exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## -O0 build and run

```
$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_ctz_o0 test_ctz.c ctz.c
./test_ctz_o0
ok   single-bit words k=0..63: mismatches 0
ok   exhaustive 16-bit (65535 values): mismatches 0
random 64-bit cases: 1000000 (zeros redrawn: 0)
identity vs shift-loop mismatches (all phases): 0
fnv64 over per-case (input, result): 0xdd36dc676b59b047
contract pin: ctz64_identity(0) = 63 (x=0 is out of contract; observed only, no guarantee)
timed sink (prevents dead-code elimination): 5006300
throughput: 5.00 ns/value (best of 5; PRNG not in timed region)
RESULT: ALL TESTS PASSED
opt0 exit=0
```

## AddressSanitizer + UBSan build and run

```
$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_ctz_asan test_ctz.c ctz.c
./test_ctz_asan
ok   single-bit words k=0..63: mismatches 0
ok   exhaustive 16-bit (65535 values): mismatches 0
random 64-bit cases: 1000000 (zeros redrawn: 0)
identity vs shift-loop mismatches (all phases): 0
fnv64 over per-case (input, result): 0xdd36dc676b59b047
contract pin: ctz64_identity(0) = 63 (x=0 is out of contract; observed only, no guarantee)
timed sink (prevents dead-code elimination): 5006300
throughput: 4.00 ns/value (best of 5; PRNG not in timed region)
RESULT: ALL TESTS PASSED
sanitize exit=0
```

The sanitizer run produced no ASan or UBSan report: zero sanitizer
findings across the full case set. The FNV-1a checksum over the
per-case (input, result) pairs, `0xdd36dc676b59b047`, is byte-identical
across all three builds.

## Disassembly check (-O2): does the compiler emit a native ctz instruction?

```
$ gcc -std=c11 -O2 -Wall -Wextra -Werror -c -o ctz.o ctz.c && objdump -d ctz.o

ctz.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <ctz64_identity>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 b9 55 55 55 55 55 	movabs $0x5555555555555555,%rcx
   b:	55 55 55
   e:	48 8d 57 ff         	lea    -0x1(%rdi),%rdx
  12:	48 31 fa            	xor    %rdi,%rdx
  15:	48 89 d0            	mov    %rdx,%rax
  18:	48 c1 ea 02         	shr    $0x2,%rdx
  1c:	48 21 ca            	and    %rcx,%rdx
  1f:	48 d1 e8            	shr    $1,%rax
  22:	48 b9 33 33 33 33 33 	movabs $0x3333333333333333,%rcx
  29:	33 33 33
  2c:	48 29 d0            	sub    %rdx,%rax
  2f:	48 89 c2            	mov    %rax,%rdx
  32:	48 c1 e8 02         	shr    $0x2,%rax
  36:	48 21 c8            	and    %rcx,%rax
  39:	48 21 ca            	and    %rcx,%rdx
  3c:	48 01 c2            	add    %rax,%rdx
  3f:	48 89 d0            	mov    %rdx,%rax
  42:	48 c1 e8 04         	shr    $0x4,%rax
  46:	48 01 d0            	add    %rdx,%rax
  49:	48 ba 0f 0f 0f 0f 0f 	movabs $0xf0f0f0f0f0f0f0f,%rdx
  50:	0f 0f 0f
  53:	48 21 d0            	and    %rdx,%rax
  56:	48 ba 01 01 01 01 01 	movabs $0x101010101010101,%rdx
  5d:	01 01 01
  60:	48 0f af c2         	imul   %rdx,%rax
  64:	48 c1 e8 38         	shr    $0x38,%rax
  68:	c3                  	ret

$ grep for native trailing-zero / popcount instructions across ctz.o and test_ctz.o:
grep exit=1 (1 = no such instruction anywhere)
```

Result, reported as measured: gcc 13.3.0 at -O2 does NOT emit a native
trailing-zero instruction (`tzcnt`/`bsf`) or `popcnt` for the identity
implementation; `ctz64_identity` stays a straight sequence of integer
arithmetic (lea/xor/shifts/ands/sub/adds, one imul for the SWAR
horizontal fold). The shift-loop reference was inlined by gcc into
`check()` and left as a literal shift-and-test loop (`shr $1` / `test` /
`jne`); it was not pattern-matched into a `bsf` either. The grep over
both object files found no `tzcnt`, `lzcnt`, `bsf`, `bsr`, or `popcnt`
anywhere.
