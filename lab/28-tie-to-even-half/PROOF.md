# PROOF.md: lab/28-tie-to-even-half

Date: 2026-09-10. Machine: x86_64, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build logs and run outputs.

`tie.c`/`tie.h` hold the implementation under test:
`tie_to_even_half()`, computed from the bit identity
`q + ((x & 1) & (q & 1))` with `q = x >> 1`. All operations are
unsigned 32-bit: two shifts, two ANDs, one add. No floats, no
division. The differential reference is the independent
quotient/remainder function `tie_to_even_half_ref()` in
`test_tie.c`, which computes `q = x / 2` and `r = x % 2` with the
division unit (a separate path from the shift/AND identity) and
resolves a tie to the even neighbor.

## Build log and run (-O2)

```
=== BUILD -O2 ===
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_tie test_tie.c tie.c
build exit=0
=== RUN -O2 ===
ok   dedicated contract rows (13): mismatches 0
ok   exhaustive u32 (4294967296 values): mismatches 0
fnv1a over per-case (input, result): 0x5ae86dd58b5d3a01
timed sink (prevents dead-code elimination): 22522923174015620
throughput: 2.29 ns/value (best of 5 over 4194304 values)
RESULT: ALL TESTS PASSED
run exit=0
```

Zero warnings under `-Wall -Wextra -Werror`.

## -O0 build and run

```
=== RUN -O0 ===
ok   dedicated contract rows (13): mismatches 0
ok   exhaustive u32 (4294967296 values): mismatches 0
fnv1a over per-case (input, result): 0x5ae86dd58b5d3a01
timed sink (prevents dead-code elimination): 22522923174015620
throughput: 3.02 ns/value (best of 5 over 4194304 values)
RESULT: ALL TESTS PASSED
```

## ASan+UBSan build and run

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_tie_asan test_tie.c tie.c
=== RUN ASan+UBSan ===
ok   dedicated contract rows (13): mismatches 0
ok   exhaustive u32 (4294967296 values): mismatches 0
fnv1a over per-case (input, result): 0x5ae86dd58b5d3a01
timed sink (prevents dead-code elimination): 22522923174015620
throughput: 2.69 ns/value (best of 5 over 4194304 values)
RESULT: ALL TESTS PASSED
```

Zero ASan/UBSan reports across the full 2^32 case set.

## Checksum agreement

The FNV-1a checksum over every case input and result,
`0x5ae86dd58b5d3a01`, is byte-identical across the `-O2`, `-O0`,
and ASan+UBSan builds. The throughput figure is recorded from the
`-O2` build.

## Disassembly (-O2, gcc 13.3.0, x86_64)

```
0000000000000000 <tie_to_even_half>:
   0:   f3 0f 1e fa             endbr64
   4:   89 f8                   mov    %edi,%eax
   6:   d1 e8                   shr    $1,%eax
   8:   21 c7                   and    %eax,%edi
   a:   83 e7 01                and    $0x1,%edi
   d:   01 f8                   add    %edi,%eax
   f:   c3                      ret
```

The function compiles to exactly the identity: `shr`, `and`, `and`,
`add`, `ret`. No `div`/`idiv` anywhere in the object.
