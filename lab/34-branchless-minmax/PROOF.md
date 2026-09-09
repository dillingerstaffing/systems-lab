# PROOF.md — lab/34-branchless-minmax

## Build

```
$ make clean && make
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_minmax test_minmax.c
exit=0
```

Clean under `-Wall -Wextra -Werror`. All randomness is a deterministic
xorshift32 with a fixed seed, so the runs below are reproducible.

## Run (-O2)

```
$ ./test_minmax
exhaustive 16-bit sweep: a=-32768 of 32767, pairs=65536, mismatches=0
exhaustive 16-bit sweep: a=-24576 of 32767, pairs=536936448, mismatches=0
exhaustive 16-bit sweep: a=-16384 of 32767, pairs=1073807360, mismatches=0
exhaustive 16-bit sweep: a=-8192 of 32767, pairs=1610678272, mismatches=0
exhaustive 16-bit sweep: a=0 of 32767, pairs=2147549184, mismatches=0
exhaustive 16-bit sweep: a=8192 of 32767, pairs=2684420096, mismatches=0
exhaustive 16-bit sweep: a=16384 of 32767, pairs=3221291008, mismatches=0
exhaustive 16-bit sweep: a=24576 of 32767, pairs=3758161920, mismatches=0
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 16.3 s
phase 2 (directed 32-bit edges): 49 pairs, 0 mismatches
phase 3 (10M random 32-bit pairs): 10000000 pairs, 0 mismatches
correctness: 4304967345 differential checks, 0 mismatches
throughput: 3.650 ns/pair over 200000000 pairs (sink=0xbeaa9e00)
ALL TESTS PASSED
exit=0
```

## Run (-O0 and ASan+UBSan)

```
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_minmax_o0 test_minmax.c && ./test_minmax_o0
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 52.3 s
phase 2 (directed 32-bit edges): 49 pairs, 0 mismatches
phase 3 (10M random 32-bit pairs): 10000000 pairs, 0 mismatches
correctness: 4304967345 differential checks, 0 mismatches
throughput: 7.835 ns/pair over 200000000 pairs (sink=0xbeaa9e00)
ALL TESTS PASSED
exit=0
```

```
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_minmax_asan test_minmax.c && ./test_minmax_asan
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 16.4 s
phase 2 (directed 32-bit edges): 49 pairs, 0 mismatches
phase 3 (10M random 32-bit pairs): 10000000 pairs, 0 mismatches
correctness: 4304967345 differential checks, 0 mismatches
throughput: 2.448 ns/pair over 200000000 pairs (sink=0xbeaa9e00)
ALL TESTS PASSED
exit=0
```

Zero warnings under all three flag sets, zero ASan/UBSan reports. The
same checksum sink (`0xbeaa9e00`) in all three timed loops confirms the
benchmarked code paths agree.

## Disassembly (-O2, non-inline wrappers, objdump -d)

```
0000000000000000 <wrap_bmin>:
   0:   f3 0f 1e fa          endbr64
   4:   48 63 c6             movslq %esi,%rax
   7:   48 63 d7             movslq %edi,%rdx
   a:   48 29 c2             sub    %rax,%rdx
   d:   89 f8                mov    %edi,%eax
   f:   48 c1 fa 3f          sar    $0x3f,%rdx
  13:   31 f0                xor    %esi,%eax
  15:   21 d0                and    %edx,%eax
  17:   31 f0                xor    %esi,%eax
  19:   c3                   ret

0000000000000020 <wrap_bmax>:
  20:   f3 0f 1e fa          endbr64
  24:   48 63 c6             movslq %esi,%rax
  27:   48 63 d7             movslq %edi,%rdx
  2a:   48 29 c2             sub    %rax,%rdx
  2d:   89 f0                mov    %esi,%eax
  2f:   48 c1 fa 3f          sar    $0x3f,%rdx
  33:   31 f8                xor    %edi,%eax
  35:   21 d0                and    %edx,%eax
  37:   31 f8                xor    %edi,%eax
  39:   c3                   ret
```

gcc 13.3.0 at `-O2` emits no conditional jump and no `cmov` in either
function: the sign bit is broadcast with `sar $0x3f` and the operand is
selected by `xor`/`and`/`xor` mask arithmetic, so every input pair
takes the identical instruction path. (Honestly noted: this is what
this compiler and flags produce; a different compiler could emit a
`cmov` or a branch, which the differential suite above would still
catch as a correctness matter but not as a codegen matter.)

## Why these numbers mean the mechanism works

The mask is bit 63 of the exact 64-bit difference, which is set exactly
when `a < b` for every `int32_t` pair: the 64-bit intermediate cannot
overflow. 4,294,967,296 exhaustive 16-bit pairs plus 49 directed
`INT32_MIN`/`INT32_MAX` edge pairs (the region where a 32-bit
`(a - b) >> 31` mask demonstrably selects the wrong operand) plus
10,000,000 random full-32-bit pairs all agree with the ternary-operator
reference, satisfy `min <= max`, and return one of the inputs:
4,304,967,345 checks, 0 mismatches, under `-O2`, `-O0`, and
ASan+UBSan. Throughput at `-O2`: 3.650 ns/pair over 200M timed pairs.
