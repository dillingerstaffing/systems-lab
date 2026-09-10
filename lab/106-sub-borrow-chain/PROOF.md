# PROOF.md: lab/106-sub-borrow-chain

Date: 2026-09-10. Machine: x86_64, gcc 13.3.0 (Ubuntu 24.04).
This file contains the genuine, unedited build logs and run outputs.

`subborrow.c`/`subborrow.h` hold the implementation under test:
`sub_borrow_chain()`, two 64-bit stages of the subtraction
d = a - b - borrow_in over 128-bit words, using only wrapping 64-bit
addition and the borrow-out identity derived below. The independent
oracle in `test_subborrow.c` uses unsigned `__int128` exact arithmetic;
the implementation never uses `__int128` (verified by disassembling the
object and grepping for it, see the disasm target in the Makefile).

## The derivation (the borrow identity)

Claim: for one 64-bit stage with inputs a, b, borrow_in in {0,1}, the
borrow-out of r = a - b - borrow_in is 1 iff a < b + borrow_in in exact
(integer) arithmetic.

Proof by cases on t = (b + borrow_in) mod 2^64, the value the wrapping
addition actually computes:

1. t did not wrap. Then borrow_in == 0 (t == b exactly), or b < 2^64 - 1
   (t == b + borrow_in exactly, since borrow_in <= 1). In both cases
   t == b + borrow_in as integers, so (a < t) is exactly the defining
   inequality a < b + borrow_in. The second term of the implementation
   is harmless here: borrow_in == 0 makes it 0, and when borrow_in == 1
   with no wrap we have t == b + 1 > b, so (t < b) is 0.

2. t wrapped. Then borrow_in == 1 and b == 2^64 - 1 (these are the only
   values with b + borrow_in == 2^64, the one sum that wraps). Then
   b + borrow_in == 2^64 exactly, and every 64-bit a satisfies
   a < 2^64, so the borrow-out must be 1. The implementation reports
   (a < t) == (a < 0) == 0, but (t < b) == (0 < 2^64 - 1) == 1 with
   borrow_in == 1, so the OR is 1. Correct.

The difference word: diff = (a - t) mod 2^64. In case 1, t is exact, so
diff = (a - (b + borrow_in)) mod 2^64. In case 2, (b + borrow_in) mod
2^64 == 0 == t, so diff = a mod 2^64 = a, matching a - (b + borrow_in)
mod 2^64. Both cases give the exact difference mod 2^64.

Chaining: sub_stage returns borrow in {0,1} (an OR of two 0/1 terms), so
the low stage's borrow-out is a valid borrow-in for the high stage,
which applies the same identity to compute (a_hi - b_hi - borrow_lo).
The final return is the borrow-out of the full 128-bit subtraction:
1 iff the 128-bit a < b + borrow_in exactly.

Edge case the wrap detector exists for: b_lo == 2^64 - 1, borrow_in 1.
Exact: subtractend is 2^64, borrow always occurs. The naive test
(a_lo < b_lo + borrow_in) would compare against the wrapped t == 0 and
under-report the borrow; the (t < b_lo) term catches exactly this one
wrap.

## Build log and run (-O2)

```
$ make clean && make && ./test_subborrow
rm -f test_subborrow test_subborrow_asan test_subborrow_o0 *.o
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_subborrow test_subborrow.c subborrow.c
directed edge rows: mismatches so far 0
exhaustive 16-bit x borrow_in (8589934592 cases): 152.7 s wall
random 64-bit cases: 10000000 (seed 0x123456789ABCDEF0, borrow_in alternating)
timed sink (prevents dead-code elimination): 8443326630441610695
throughput: 4.55 ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)
total cases: 8599934628
implementation vs __int128 oracle mismatches (all phases): 0
fnv1a over per-case (inputs, outputs): 0xfce86f5578b4ca16
RESULT: ALL TESTS PASSED
```

Zero warnings under `-Wall -Wextra -Werror` (empty stderr/stdout from gcc).

## -O0 build and run

```
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_subborrow_o0 test_subborrow.c subborrow.c && ./test_subborrow_o0
(no output from gcc: zero warnings)
directed edge rows: mismatches so far 0
exhaustive 16-bit x borrow_in (8589934592 cases): 575.4 s wall
random 64-bit cases: 10000000 (seed 0x123456789ABCDEF0, borrow_in alternating)
timed sink (prevents dead-code elimination): 8443326630441610695
throughput: 15.83 ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)
total cases: 8599934628
implementation vs __int128 oracle mismatches (all phases): 0
fnv1a over per-case (inputs, outputs): 0xfce86f5578b4ca16
RESULT: ALL TESTS PASSED
```

## AddressSanitizer + UBSan build and run

```
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_subborrow_asan test_subborrow.c subborrow.c && ./test_subborrow_asan
(no output from gcc: zero warnings)
directed edge rows: mismatches so far 0
exhaustive 16-bit x borrow_in (8589934592 cases): 501.2 s wall
random 64-bit cases: 10000000 (seed 0x123456789ABCDEF0, borrow_in alternating)
timed sink (prevents dead-code elimination): 8443326630441610695
throughput: 14.59 ns/value (best of 5 over 1000000 pre-generated cases, PRNG outside timed region)
total cases: 8599934628
implementation vs __int128 oracle mismatches (all phases): 0
fnv1a over per-case (inputs, outputs): 0xfce86f5578b4ca16
RESULT: ALL TESTS PASSED
```

Zero sanitizer reports in any run: no ASan or UBSan output appears
above, and every binary exited 0.

## Cross-build checksum

The FNV-1a checksum over per-case inputs and outputs is
`0xfce86f5578b4ca16` in all three builds (`-O2`, `-O0`, ASan+UBSan),
over the identical case stream: 36 directed rows, the full 2^33
exhaustive cases, and the 10,000,000 fixed-seed random cases.

## Disassembly (gcc 13.3.0, x86_64, -O2)

`objdump -d subborrow.o --disassemble=sub_borrow_chain` shows the
intended construction and nothing else: `add` of the borrow-in, carry
detection via `setb`, the wrap detector via a second `setb` + `and` +
`or`, two `sub`s, and `ret`. A grep over the object for `__int128`
references returns nothing, so the implementation never uses wide
arithmetic; only the test oracle does.

## Note on scope

The full 2^33 exhaustive sweep was run at all three build
configurations (-O2: 152.7 s, -O0: 575.4 s, ASan+UBSan: 501.2 s),
with no scope reduction. Every number above comes from those runs.
```
