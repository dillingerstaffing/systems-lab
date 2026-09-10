<!-- PROOF-HEADER
Checks: 1065537
Mismatches: 0
Checksum: 0x6f25fc9c8181575a
Throughput: 0.594 ns/value at -O2, best of 5
Verdict: PASS
-->
# PROOF.md: lab/81-wrapping-neg

Genuine build log and run output, captured 2026-09-10.  Toolchain:
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.  Flags:
`-std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L`.

## Build log

```
$ make all
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='\"-O0\"' -o test_wrapneg_o0 test_wrapneg.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='\"-O2\"' -o test_wrapneg_o2 test_wrapneg.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address \
	-fno-omit-frame-pointer -DBUILD_NAME='\"asan\"' -o test_wrapneg_asan test_wrapneg.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='\"ubsan\"' -o test_wrapneg_ubsan test_wrapneg.c
```

No warnings, no errors (`-Werror` was in force for all four builds).

## Run output, -O2 build (exit 0)

```
wrapneg differential test, build -O2
[1/5] exhaustive 16-bit inputs (zero-extended)
  done: cases=65536 mismatches=0 violations=0
[2/5] random 64-bit words, splitmix64 seed 0x123456789ABCDEF0
  done: cases=1000000 mismatches=0 violations=0
[3/5] INT64_MIN edge: wrapneg(0x8000000000000000)
  wrapneg(INT64_MIN)=8000000000000000 (contract: equal to input)
[4/5] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan, UBSan
[5/5] throughput (PRNG pre-generated, excluded from timing)
  pass 0: 0.631 ns/value (1585.201 M values/s) sink=96bc598bfafa4458
  pass 1: 0.597 ns/value (1674.685 M values/s) sink=96bc598bfafa4458
  pass 2: 0.594 ns/value (1683.042 M values/s) sink=96bc598bfafa4458
  pass 3: 0.596 ns/value (1678.544 M values/s) sink=96bc598bfafa4458
  pass 4: 0.720 ns/value (1389.190 M values/s) sink=96bc598bfafa4458
  throughput best of 5: 0.594 ns/value (1683.042 M values/s)
  with PRNG in the timed loop (ceiling): 1.607 ns/value (sink=e16d0f0c4f42b7ff)
total verification cases: 1065537
total mismatches: 0
invariant violations: 0
FNV-1a checksum of all outputs: 0x6f25fc9c8181575a
RESULT: PASS
```

## Run output, -O0 build (exit 0)

```
wrapneg differential test, build -O0
[1/5] exhaustive 16-bit inputs (zero-extended)
  done: cases=65536 mismatches=0 violations=0
[2/5] random 64-bit words, splitmix64 seed 0x123456789ABCDEF0
  done: cases=1000000 mismatches=0 violations=0
[3/5] INT64_MIN edge: wrapneg(0x8000000000000000)
  wrapneg(INT64_MIN)=8000000000000000 (contract: equal to input)
[4/5] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan, UBSan
[5/5] throughput (PRNG pre-generated, excluded from timing)
  pass 0: 2.700 ns/value (370.332 M values/s) sink=96bc598bfafa4458
  pass 1: 2.771 ns/value (360.858 M values/s) sink=96bc598bfafa4458
  pass 2: 2.744 ns/value (364.394 M values/s) sink=96bc598bfafa4458
  pass 3: 4.662 ns/value (214.490 M values/s) sink=96bc598bfafa4458
  pass 4: 4.584 ns/value (218.130 M values/s) sink=96bc598bfafa4458
  throughput best of 5: 2.700 ns/value (370.332 M values/s)
  with PRNG in the timed loop (ceiling): 5.145 ns/value (sink=e16d0f0c4f42b7ff)
total verification cases: 1065537
total mismatches: 0
invariant violations: 0
FNV-1a checksum of all outputs: 0x6f25fc9c8181575a
RESULT: PASS
```

## Run summary, ASan build (exit 0)

Sections [1] through [4] identical to above: 65536 exhaustive
cases, 1,000,000 random cases, the INT64_MIN edge row, 0
mismatches, 0 invariant violations.  Throughput best of 5:
0.637 ns/value (1570.079 M values/s); with PRNG in the timed
loop (ceiling): 2.832 ns/value.  Sinks identical:
`96bc598bfafa4458` and `e16d0f0c4f42b7ff`.
FNV-1a checksum of all outputs: `0x6f25fc9c8181575a`.
RESULT: PASS.  No sanitizer reports.

## Run summary, UBSan build (exit 0)

Sections [1] through [4] identical to above: 65536 exhaustive
cases, 1,000,000 random cases, the INT64_MIN edge row, 0
mismatches, 0 invariant violations.  Throughput best of 5:
1.071 ns/value (933.791 M values/s); with PRNG in the timed
loop (ceiling): 3.263 ns/value.  Sinks identical:
`96bc598bfafa4458` and `e16d0f0c4f42b7ff`.
FNV-1a checksum of all outputs: `0x6f25fc9c8181575a`.
RESULT: PASS.  No sanitizer reports.

## Notes

- The checksum `0x6f25fc9c8181575a` is identical across `-O0`,
  `-O2`, ASan, and UBSan: the same outputs, bit for bit, under
  no optimization, full optimization, and both instrumentations.
  It covers every output produced in sections [1] through [3]
  (the timed loops run after the checksum is complete and use
  separate sinks).
- Throughput at `-O2`, best of 5 over 100,000,000 timed values
  with the splitmix64 PRNG pre-generated into a 2^20-entry
  buffer and excluded from the timed loop: 0.594 ns/value
  (1683.042 M values/s).  With the PRNG step inside the timed
  loop the cost rises to 1.607 ns/value, so that figure is a
  ceiling that includes the generator cost.  The per-pass sinks
  are identical across all passes and all four builds,
  confirming the timed loops computed the same values
  everywhere.
- The differential check compares against unary minus for every
  case except INT64_MIN, where signed negation is undefined in
  C; that one input is pinned as a dedicated row with an
  explicit contract.  The two invariants
  (`x + neg(x) == 0`, `neg(neg(x)) == x`) are checked on all
  1,065,537 cases including INT64_MIN.
- The implementation uses only unsigned 64-bit NOT and
  addition; no builtins, no intrinsics, no inline asm, no
  library calls, no unary minus in `wrapneg.h`.
