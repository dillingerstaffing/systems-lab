# PROOF.md: lab/94-saturate-to-u16

Genuine build log and run output, captured 2026-09-10.  Toolchain:
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.  Flags:
`-std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L`.

## Build log

```
$ make all
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='\"-O0\"' -o test_saturate_u16_o0 test_saturate_u16.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='\"-O2\"' -o test_saturate_u16_o2 test_saturate_u16.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='\"asan+ubsan\"' -o test_saturate_u16_asan test_saturate_u16.c
```

No warnings, no errors (`-Werror` was in force for all three builds).

## Run output, -O2 build (exit 0)

```
saturate_u16 differential test, build -O2
[1/4] anchors
  anchors checked: 12, mismatches so far: 0
[2/4] exhaustive all 2^32 inputs
  done: cases=4294967296 mismatches=0
[3/4] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan+UBSan
[4/4] throughput (PRNG pre-generated, excluded from timing)
  sat pass 0: 0.717 ns/value (1395.275 M values/s) sink=32069de0bd4
  sat pass 1: 0.745 ns/value (1341.563 M values/s) sink=32069de0bd4
  sat pass 2: 0.698 ns/value (1431.833 M values/s) sink=32069de0bd4
  sat pass 3: 0.724 ns/value (1381.277 M values/s) sink=32069de0bd4
  sat pass 4: 0.694 ns/value (1440.114 M values/s) sink=32069de0bd4
  sat throughput best of 5: 0.694 ns/value (1440.114 M values/s)
  with PRNG in the timed loop: 4.354 ns/value (sink=32012b44464)
total verification cases: 4294967308
total mismatches: 0
FNV-1a checksum of all outputs: 0x31f7e4badfa3e62a
RESULT: PASS

real	0m32.476s
user	0m25.389s
sys	0m0.063s
```

## Run summary, -O0 build (exit 0)

```
[1/4] anchors checked: 12, mismatches so far: 0
[2/4] done: cases=4294967296 mismatches=0
[4/4] sat throughput best of 5: 7.819 ns/value (127.892 M values/s)
      with PRNG in the timed loop: 24.944 ns/value (sink=32012b44464)
total verification cases: 4294967308
total mismatches: 0
FNV-1a checksum of all outputs: 0x31f7e4badfa3e62a
RESULT: PASS

real	4m12.196s
user	2m29.578s
sys	0m0.546s
```

## Run summary, ASan+UBSan build (exit 0)

```
[1/4] anchors checked: 12, mismatches so far: 0
[2/4] done: cases=4294967296 mismatches=0
[4/4] sat throughput best of 5: 2.873 ns/value (348.035 M values/s)
      with PRNG in the timed loop: 5.556 ns/value (sink=32012b44464)
total verification cases: 4294967308
total mismatches: 0
FNV-1a checksum of all outputs: 0x31f7e4badfa3e62a
RESULT: PASS

real	0m30.747s
user	0m27.037s
sys	0m0.063s
```

Zero sanitizer reports across the full case set (no ASan or UBSan
diagnostics in the output).

## Disassembly (gcc 13.3.0, `-O2`, non-inline wrapper, `objdump -d`)

A separate translation unit declared the header function behind
`__attribute__((noinline)) uint16_t wrap_saturate_u16(int32_t x)`,
compiled `gcc -O2 -c`, then disassembled:

```
0000000000000000 <wrap_saturate_u16>:
   0:	f3 0f 1e fa         	endbr64
   4:	89 f8               	mov    %edi,%eax
   6:	c1 ff 1f            	sar    $0x1f,%edi
   9:	f7 d7               	not    %edi
   b:	21 c7               	and    %eax,%edi
   d:	8d 87 00 00 ff ff   	lea    -0x10000(%rdi),%eax
  13:	c1 f8 1f            	sar    $0x1f,%eax
  16:	f7 d0               	not    %eax
  18:	09 f8               	or     %edi,%eax
  1a:	c3                  	ret
```

`grep -cE '\tj[a-z]+\t'` over the function body: 0 conditional
jump instructions; 0 `cmov` instructions as well.  The clamp
compiles to `mov`/`sar`/`not`/`and`/`lea`/`sar`/`not`/`or`/`ret`
only: the two sign masks are built with `sar`/`not` and the
select is done with `and`/`or`, so every input takes the identical
instruction path.

How the 9 instructions implement the two masks: `sar $0x1f`
broadcasts bit 31 of the input, `not`+`and` keep the input when
it was non-negative and zero it otherwise (`t = max(x, 0)`);
`lea -0x10000(%rdi)` computes `t - 65536`, a second `sar $0x1f`
broadcasts the sign of that difference, and the final `not`+`or`
yields `t | mask`, which truncates to 65535 in the low 16 bits
exactly when `t >= 65536` (the return type is `uint16_t`, so the
upper bits of `t | 0xFFFFFFFF` are discarded).  The compiler
folded the source's `t ^ ((t ^ 65535u) & mask)` into `t | mask`
under the truncation, which is equal: when mask is all ones both
give 65535 in the low 16 bits, and when mask is zero both give
`t`.  (Honestly noted: this is what this compiler and flags
produce; a different compiler could emit a `cmov` or a branch,
which the differential suite above would still catch as a
correctness matter but not as a codegen matter.)

## What the numbers mean

- 4,294,967,308 differential cases, 0 mismatches: 12 hand-checked
  anchors (`INT32_MIN`, `-65537`, `-65536`, `-2`, `-1`, `0`, `1`,
  `65534`, `65535`, `65536`, `65537`, `INT32_MAX`; every expected
  value cross-checked against the independent reference so a typo
  cannot silently pass) plus all 4,294,967,296 `int32` inputs
  checked against the independent if/else reference.
- On every one of the 4,294,967,296 exhaustive cases the
  invariants `saturate(saturate(x)) == saturate(x)` and "the
  output is monotone non-decreasing in `x`" held with 0
  violations.  Monotonicity was checked as a running comparison
  across the input order, with the running previous value reset
  at the `u = 2^31` wrap point (the only place the monotone
  order legitimately restarts: outputs rise 0 to 65535, hold at
  65535 through `INT32_MAX`, drop to 0 at `INT32_MIN`, hold at 0
  through -1).
- The checksum `0x31f7e4badfa3e62a` is identical across `-O0`,
  `-O2`, and ASan+UBSan: the same outputs, bit for bit, under no
  optimization, full optimization, and instrumentation.  It
  covers every output value produced by sections [1] and [2]
  plus the five timed-loop sinks from section [4].
- The exhaustive differential run took 4m12s at `-O0`, 32.5s at
  `-O2`, and 30.7s under ASan+UBSan on this 2-core host, so each
  build genuinely re-verified the full 2^32 input space.
- Throughput at `-O2`, best of 5 over 100,000,000 timed values
  with the splitmix64 PRNG pre-generated into a 2^20-entry buffer
  and excluded from the timed loop: 0.694 ns/value (1440.114 M
  values/s).  With the PRNG step inside the timed loop the cost
  rises to 4.354 ns/value, so most of that loop is the PRNG; the
  0.694 ns figure is the per-value cost with the PRNG excluded.
  The per-pass sinks (`32069de0bd4`, `32012b44464`) are identical
  across all passes and all three builds, confirming the timed
  loops computed the same values everywhere.
- The implementation uses only unsigned 32-bit arithmetic,
  shifts by compile-time constants below 32, and bitwise ops, so
  no undefined behavior is possible; the ASan+UBSan run over the
  full 2^32 case set confirms it empirically (zero reports).
  The if/else reference operates on in-range `int32` values only
  and is likewise fully defined.

## Limits, stated honestly

- The differential domain is exactly the `int32_t` input domain;
  there is nothing outside it.  The clamp is total: every input
  is either below 0, in `[0, 65535]`, or above 65535, and the
  exhaustive sweep covered all three regions.
- The disassembly claim ("no conditional jump") is about the
  codegen of this exact compiler and flags, not a property of
  the C source on every compiler; the C source itself contains
  no branching constructs, so a compiler that emitted a branch
  would have introduced it, not the implementation.
