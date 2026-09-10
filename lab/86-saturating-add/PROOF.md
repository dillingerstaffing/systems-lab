# PROOF.md — lab/86-saturating-add

## Build

```
$ make clean && make
rm -f test_satadd test_satadd_asan test_satadd_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_satadd test_satadd.c
exit=0
```

```
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_satadd_o0 test_satadd.c
exit=0
```

```
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_satadd_asan test_satadd.c
exit=0
```

Zero warnings under `-Wall -Wextra -Werror` on all three flag sets.
All randomness is a deterministic splitmix64 with a fixed seed
(`0x9E3779B97F4A7C15`), so the runs below are reproducible.

## Run (-O2)

```
$ ./test_satadd
phase 0 (known answers): 11 checks, 0 mismatches
phase 1 (1M fixed-seed random pairs): 1000000 pairs, 0 mismatches
phase 2 (directed edge pairs): 65536 pairs, 0 mismatches
correctness: 1065547 differential checks, 0 mismatches
checksum (FNV-1a over all results): 0x6e1b8f0f3391f907
throughput: 5.146 ns/pair over 200000000 timed pairs (best of 5, timed checksum=0x093bb573da86c1d3)
ALL TESTS PASSED
exit=0
```

## Run (-O0)

```
$ ./test_satadd_o0
phase 0 (known answers): 11 checks, 0 mismatches
phase 1 (1M fixed-seed random pairs): 1000000 pairs, 0 mismatches
phase 2 (directed edge pairs): 65536 pairs, 0 mismatches
correctness: 1065547 differential checks, 0 mismatches
checksum (FNV-1a over all results): 0x6e1b8f0f3391f907
throughput: 12.408 ns/pair over 200000000 timed pairs (best of 5, timed checksum=0x093bb573da86c1d3)
ALL TESTS PASSED
exit=0
```

## Run (ASan+UBSan)

```
$ ./test_satadd_asan
phase 0 (known answers): 11 checks, 0 mismatches
phase 1 (1M fixed-seed random pairs): 1000000 pairs, 0 mismatches
phase 2 (directed edge pairs): 65536 pairs, 0 mismatches
correctness: 1065547 differential checks, 0 mismatches
checksum (FNV-1a over all results): 0x6e1b8f0f3391f907
throughput: 6.165 ns/pair over 200000000 timed pairs (best of 5, timed checksum=0x093bb573da86c1d3)
ALL TESTS PASSED
exit=0
```

Zero ASan/UBSan reports. The identical FNV-1a checksums
(`0x6e1b8f0f3391f907` over all correctness results,
`0x093bb573da86c1d3` over the timed runs) across `-O2`, `-O0`, and
ASan+UBSan confirm all three builds compute the identical answers.

## Disassembly (-O2, non-inline wrapper, objdump -d)

```
0000000000000000 <wrap_sat_add32>:
   0:   f3 0f 1e fa          endbr64
   4:   8d 14 37             lea    (%rdi,%rsi,1),%edx
   7:   89 f0                mov    %esi,%eax
   9:   89 fe                mov    %edi,%esi
   b:   31 d0                xor    %edx,%eax
   d:   31 d6                xor    %edx,%esi
   f:   21 c6                and    %eax,%esi
  11:   89 f8                mov    %edi,%eax
  13:   c1 f8 1f             sar    $0x1f,%eax
  16:   c1 fe 1f             sar    $0x1f,%esi
  19:   35 ff ff ff 7f       xor    $0x7fffffff,%eax
  1e:   21 f0                and    %esi,%eax
  20:   f7 d6                not    %esi
  22:   21 d6                and    %edx,%esi
  24:   09 f0                or     %esi,%eax
  26:   c3                   ret
```

gcc 13.3.0 at `-O2` emits no conditional jump and no `cmov`: the
overflow flag becomes a mask broadcast by `sar $0x1f`, and the result
is selected by `xor`/`and`/`not`/`or` mask arithmetic, so every input
pair takes the identical instruction path. (Honestly noted: this is
what this compiler and flags produce; a different compiler could emit
a branch or `cmov`, which the differential suite above would still
catch as a correctness matter but not as a codegen matter.)

## Why these numbers mean the mechanism works

The overflow detector is bit 31 of `((ua ^ sum) & (ub ^ sum))`. It is
set exactly when `sign(a) == sign(b) != sign(sum)`, which is the
definition of two's complement signed-overflow for addition. The
implementation never performs a signed operation that could overflow:
the only addition is unsigned (defined to wrap), the shifts are on
unsigned values, and the final `uint32_t` to `int32_t` conversion runs
only on a value already known to be in range (the wrapped sum when
there is no overflow, `INT32_MIN`/`INT32_MAX` when there is), so the
conversion is exact, never implementation-defined.

1,000,000 fixed-seed random full-range pairs plus 65,536 directed edge
pairs (the cross product of 256 boundary values: 32 at each of
`INT32_MIN + i`, `INT32_MAX - i`, `-32768 + i`, `32767 - i`, plus
`-64`..`63`, putting `INT32_MAX + 1`, `INT32_MIN - 1`, and every
saturation corner in both operand slots) plus 11 known-answer clamp
corners all agree with the `int64_t` oracle: 1,065,547 checks,
0 mismatches, under `-O2`, `-O0`, and ASan+UBSan. Throughput at
`-O2`: 5.146 ns/pair over 200M timed pairs (best of 5). Honest caveat:
the timed loop includes the splitmix64 PRNG draw and the FNV-1a fold
per pair, so 5.146 ns covers the whole pair pipeline, not the
`sat_add32` call alone.
