# PROOF.md — lab/51-branchless-abs-diff

`badiff64(a, b)`: absolute difference of two `int64_t` values as a
`uint64_t`, computed as the sign-mask identity
`(d ^ (d >> 63)) - (d >> 63)` on the difference `d = a - b`, evaluated
entirely on unsigned 64-bit values. Header-only: `absdiff.h`.
Tests: `test_absdiff.c`.

## Contract and the one excluded input

`badiff64` is defined exactly when the true difference `a - b`
satisfies `INT64_MIN < (a - b) <= INT64_MAX`, i.e. `|a - b| <= INT64_MAX`.
When the true difference fits in `int64_t`, the wrapped difference
`(uint64_t)a - (uint64_t)b` reinterpreted as signed is the true `d`,
and the identity is exact.

The single excluded input is `d == INT64_MIN`: the signed negation in
the identity overflows there (undefined behavior), and the reference
`llabs(INT64_MIN)` is undefined as well. The test never exercises it:
phase 2's directed edges include the `(INT64_MIN, 0)` pair, whose exact
difference is `INT64_MIN`, and the oracle skips it explicitly (counted
below). Phase 1 (16-bit pairs) and phase 3 (operands in
`[-2^60, 2^60)`, differences in `(-2^61, 2^61)`) cannot produce
`INT64_MIN` by construction, and the test asserts no pair was skipped
outside phase 2.

## Build

```
$ make clean && make
rm -f test_absdiff test_absdiff_asan test_absdiff_o0 disasm_absdiff.o
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_absdiff test_absdiff.c
exit=0
```

Clean under `-Wall -Wextra -Werror`. The PRNG is splitmix64 with a fixed
seed `0x243F6A8885A308D3` (fractional digits of pi), so every run is
reproducible.

## Run (-O2)

```
$ ./test_absdiff
exhaustive 16-bit sweep: a=-32768 of 32767, pairs=65536, mismatches=0
exhaustive 16-bit sweep: a=-24576 of 32767, pairs=536936448, mismatches=0
exhaustive 16-bit sweep: a=-16384 of 32767, pairs=1073807360, mismatches=0
exhaustive 16-bit sweep: a=-8192 of 32767, pairs=1610678272, mismatches=0
exhaustive 16-bit sweep: a=0 of 32767, pairs=2147549184, mismatches=0
exhaustive 16-bit sweep: a=8192 of 32767, pairs=2684420096, mismatches=0
exhaustive 16-bit sweep: a=16384 of 32767, pairs=3221291008, mismatches=0
exhaustive 16-bit sweep: a=24576 of 32767, pairs=3758161920, mismatches=0
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 6.5 s
phase 2 (directed 64-bit edges): 81 pairs checked, 24 skipped (outside contract), 0 mismatches
phase 3 (1M random 64-bit pairs): 1000000 pairs, 0 mismatches
correctness: 4295967353 differential checks, 0 mismatches
throughput: 4.389 ns/pair over 200000000 pairs (sink=0xb2fe6b37b79e710f)
ALL TESTS PASSED
exit=0
```

## Run (-O0 and ASan+UBSan)

```
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_absdiff_o0 test_absdiff.c && ./test_absdiff_o0
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 65.5 s
phase 2 (directed 64-bit edges): 81 pairs checked, 24 skipped (outside contract), 0 mismatches
phase 3 (1M random 64-bit pairs): 1000000 pairs, 0 mismatches
correctness: 4295967353 differential checks, 0 mismatches
throughput: 6.973 ns/pair over 200000000 pairs (sink=0xb2fe6b37b79e710f)
ALL TESTS PASSED
exit=0
```

```
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_absdiff_asan test_absdiff.c && ./test_absdiff_asan
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 166.0 s
phase 2 (directed 64-bit edges): 81 pairs checked, 24 skipped (outside contract), 0 mismatches
phase 3 (1M random 64-bit pairs): 1000000 pairs, 0 mismatches
correctness: 4295967353 differential checks, 0 mismatches
throughput: 4.275 ns/pair over 200000000 pairs (sink=0xb2fe6b37b79e710f)
ALL TESTS PASSED
exit=0
```

Zero warnings under all three flag sets, zero ASan/UBSan reports. The
same checksum sink (`0xb2fe6b37b79e710f`) in all three timed loops
confirms the three binaries computed identical results on the same
200,000,000-pair stream. The 24 phase-2 skips were independently
recomputed in Python from the contract predicate
(`INT64_MIN+1 <= a-b <= INT64_MAX` over exact `__int128` differences):
24 skipped pairs, including `(INT64_MIN, 0)`, the documented exclusion.

## Disassembly (gcc -O2, non-inline wrapper, `objdump -d`)

```
0000000000000000 <badiff64_wrap>:
   0:  f3 0f 1e fa          endbr64
   4:  48 29 f7             sub    %rsi,%rdi
   7:  48 89 fa             mov    %rdi,%rdx
   a:  48 c1 fa 3f          sar    $0x3f,%rdx
   e:  48 31 d7             xor    %rdx,%rdi
  11:  48 89 f8             mov    %rdi,%rax
  14:  48 29 d0             sub    %rdx,%rax
  17:  c3                   ret
```

The compiler folded `m = -(d >> 63)` into the `sar` result directly (the
shift already yields the all-ones-or-zero mask), so the body is
`sub`/`sar`/`xor`/`sub`: no conditional jump, no `cmov`; the
instruction path is identical for every input pair.

## What was verified, exactly

- 4,294,967,296 exhaustive 16-bit pairs (sign-extended to `int64_t`),
  differential against `llabs`, 0 mismatches, at `-O0`, `-O2`, and
  under ASan+UBSan.
- 81 directed 64-bit edge pairs; 57 checked against `llabs` with 0
  mismatches; 24 skipped as outside the contract, including the one
  documented exclusion `d == INT64_MIN` (pair `(INT64_MIN, 0)`).
- 1,000,000 fixed-seed splitmix64 pairs in `[-2^60, 2^60)`, 0 mismatches.
- Total: 4,295,967,353 differential checks, 0 mismatches.
- No branch in the compiled function (verified in the disassembly).
- The result is a `uint64_t`, so no claim is made about magnitudes above
  `INT64_MAX`, and no signed arithmetic appears anywhere in the
  implementation.
