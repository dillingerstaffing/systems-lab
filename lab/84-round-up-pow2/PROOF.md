<!-- PROOF-HEADER
Mismatches: 0
Checksum: 0x45777d315651eb4c
Throughput: 2.27 ns/value at -O2, best of 5
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/84-round-up-pow2

Date: 2026-09-10. Machine: x86_64, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build logs and run outputs.

`roundup.c`/`roundup.h` hold the implementation under test:
`round_up_pow2_64()`, built only from the shift/OR fill cascade
`v |= v >> k` (k = 1,2,4,8,16,32) applied to `(x - 1)`, then `+ 1`.
The differential reference is the independent division loop
`round_up_pow2_64_ref()` in `test_roundup.c`, which computes the
smallest power of two >= x by repeated doubling and returns 0 when
the next doubling would exceed 64 bits (i.e. inputs above 2^63).

One test-binary expectation was corrected during the run: the row
`2^k - 1` for k = 1 gives x = 1, which is itself a power of two, so
the correct expectation is the identity (1), not 2. The row set now
covers `2^k - 1` for k = 2..63; the fix touched only the test file,
not the implementation, and the first failing run was discarded
(the final binary was rebuilt from the fixed source).

## Build log and run (-O2)

```
$ make clean && make
=== BUILD -O2 ===
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_roundup test_roundup.c roundup.c
make exit=0
=== RUN -O2 ===
ok   dedicated contract rows (194): mismatches 0
ok   exhaustive 16-bit (65536 values): mismatches 0
random 64-bit cases: 1000000 (seed 0x123456789ABCDEF0)
implementation vs division-loop reference mismatches (all phases): 0
fnv1a over per-case (input, result): 0x45777d315651eb4c
timed sink (prevents dead-code elimination): 14621393587095470080
throughput: 2.27 ns/value (best of 5; PRNG not in timed region)
RESULT: ALL TESTS PASSED
run exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## -O0 build and run

```
$ make opt0
=== BUILD -O0 ===
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_roundup_o0 test_roundup.c roundup.c
./test_roundup_o0
ok   dedicated contract rows (194): mismatches 0
ok   exhaustive 16-bit (65536 values): mismatches 0
random 64-bit cases: 1000000 (seed 0x123456789ABCDEF0)
implementation vs division-loop reference mismatches (all phases): 0
fnv1a over per-case (input, result): 0x45777d315651eb4c
timed sink (prevents dead-code elimination): 14621393587095470080
throughput: 4.81 ns/value (best of 5; PRNG not in timed region)
RESULT: ALL TESTS PASSED
opt0 exit=0
```

## AddressSanitizer + UBSan build and run

```
$ make sanitize
=== BUILD ASan+UBSan ===
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_roundup_asan test_roundup.c roundup.c
./test_roundup_asan
ok   dedicated contract rows (194): mismatches 0
ok   exhaustive 16-bit (65536 values): mismatches 0
random 64-bit cases: 1000000 (seed 0x123456789ABCDEF0)
implementation vs division-loop reference mismatches (all phases): 0
fnv1a over per-case (input, result): 0x45777d315651eb4c
timed sink (prevents dead-code elimination): 14621393587095470080
throughput: 3.53 ns/value (best of 5; PRNG not in timed region)
RESULT: ALL TESTS PASSED
sanitize exit=0
```

Zero sanitizer reports across the full case set. The FNV-1a checksum
over all case inputs and results is identical in all three builds:
`0x45777d315651eb4c`.

## Disassembly (-O2, gcc 13.3.0, x86_64)

```
$ make disasm
gcc -std=c11 -O2 -Wall -Wextra -Werror -c -o roundup.o roundup.c
--- round_up_pow2_64 disassembly ---
objdump -d roundup.o --disassemble=round_up_pow2_64

roundup.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <round_up_pow2_64>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 83 ef 01         	sub    $0x1,%rdi
   8:	48 89 f8            	mov    %rdi,%rax
   b:	48 d1 e8            	shr    $1,%rax
   e:	48 09 f8            	or     %rdi,%rax
  11:	48 89 c7            	mov    %rax,%rdi
  14:	48 c1 ef 02         	shr    $0x2,%rdi
  18:	48 09 c7            	or     %rax,%rdi
  1b:	48 89 fa            	mov    %rdi,%rdx
  1e:	48 c1 ea 04         	shr    $0x4,%rdx
  22:	48 09 fa            	or     %rdi,%rdx
  25:	48 89 d0            	mov    %rdx,%rax
  28:	48 c1 e8 08         	shr    $0x8,%rax
  2c:	48 09 d0            	or     %rdx,%rax
  2f:	48 89 c2            	mov    %rax,%rdx
  32:	48 c1 ea 10         	shr    $0x10,%rdx
  36:	48 09 c2            	or     %rax,%rdx
  39:	48 89 d0            	mov    %rdx,%rax
  3c:	48 c1 e8 20         	shr    $0x20,%rax
  40:	48 09 d0            	or     %rdx,%rax
  43:	48 83 c0 01         	add    $0x1,%rax
  47:	c3                  	ret
--- builtin-instruction grep (expect no output) ---
objdump -d roundup.o | grep -Ei 'tzcnt|bsf|popcnt|l zcnt' || true
```

The compiled function is exactly the intended construction: one
subtract, five shift/OR pairs (1, 2, 4, 8, 16, 32), one add, return.
The grep for `tzcnt`, `bsf`, `popcnt`, `lzcnt` found nothing.
