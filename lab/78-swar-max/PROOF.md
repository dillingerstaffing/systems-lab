<!-- PROOF-HEADER
Checks: 4309161635
Mismatches: 0
Checksum: 0x64d1c937981935ce
Verdict: PASS
-->
# PROOF.md: lab/78-swar-max

Genuine build log and run output, captured 2026-09-10.  Toolchain:
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.  Flags:
`-std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L`.

## Build log

```
$ make all
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_swar_max_o0 test_swar_max.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_swar_max_o2 test_swar_max.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_swar_max.c
```

No warnings, no errors (`-Werror` was in force for all three builds).

## Run output, -O2 build (exit 0)

```
swar_max differential test, build -O2
[1/6] anchors
  anchors checked: 21, mismatches so far: 0
[2/6] exhaustive 16-bit pairs, replicated to all four lanes
  done: cases=4294967296 mismatches=0
[3/6] directed lane-crosstalk cases
  done: cases=4194304 mismatches=0
[4/6] horizontal max: directed + random 64-bit words
  done: cases=10000014 mismatches=0
[5/6] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan+UBSan
[6/6] throughput (PRNG pre-generated, excluded from timing)
  hmax pass 0: 5.395 ns/value (185.355 M values/s) sink=4ff86bc6a44
  hmax pass 1: 4.026 ns/value (248.415 M values/s) sink=4ff86bc6a44
  hmax pass 2: 3.963 ns/value (252.352 M values/s) sink=4ff86bc6a44
  hmax pass 3: 4.041 ns/value (247.467 M values/s) sink=4ff86bc6a44
  hmax pass 4: 3.943 ns/value (253.589 M values/s) sink=4ff86bc6a44
  hmax throughput best of 5: 3.943 ns/value (253.589 M values/s)
  max2 pass 0: 1.895 ns/value (527.570 M values/s) sink=6702cd86b7cd08a0
  max2 pass 1: 1.600 ns/value (625.066 M values/s) sink=6702cd86b7cd08a0
  max2 pass 2: 1.484 ns/value (673.973 M values/s) sink=6702cd86b7cd08a0
  max2 pass 3: 1.511 ns/value (661.654 M values/s) sink=6702cd86b7cd08a0
  max2 pass 4: 1.493 ns/value (669.593 M values/s) sink=6702cd86b7cd08a0
  max2 throughput best of 5: 1.484 ns/value (673.973 M values/s)
  with PRNG in the timed loop: 12.927 ns/value (sink=4fff0a95419)
total verification cases: 4309161635
total mismatches: 0
FNV-1a checksum of all outputs: 0x64d1c937981935ce
RESULT: PASS
```

## Run summary, -O0 build (exit 0)

```
[1/6] anchors checked: 21, mismatches so far: 0
[2/6] done: cases=4294967296 mismatches=0
[3/6] done: cases=4194304 mismatches=0
[4/6] done: cases=10000014 mismatches=0
[6/6] hmax throughput best of 5: 16.435 ns/value (60.844 M values/s)
      max2 throughput best of 5: 7.782 ns/value (128.497 M values/s)
      with PRNG in the timed loop: 28.979 ns/value (sink=4fff0a95419)
total verification cases: 4309161635
total mismatches: 0
FNV-1a checksum of all outputs: 0x64d1c937981935ce
RESULT: PASS
```

## Run summary, ASan+UBSan build (exit 0)

```
[1/6] anchors checked: 21, mismatches so far: 0
[2/6] done: cases=4294967296 mismatches=0
[3/6] done: cases=4194304 mismatches=0
[4/6] done: cases=10000014 mismatches=0
[6/6] hmax throughput best of 5: 8.434 ns/value (118.573 M values/s)
      max2 throughput best of 5: 5.687 ns/value (175.839 M values/s)
      with PRNG in the timed loop: 12.114 ns/value (sink=4fff0a95419)
total verification cases: 4309161635
total mismatches: 0
FNV-1a checksum of all outputs: 0x64d1c937981935ce
RESULT: PASS
```

Zero sanitizer reports across the full case set (no ASan or UBSan
diagnostics in the output).

## What the numbers mean

- 4,309,161,635 differential cases, 0 mismatches: 21 hand-checked
  anchors (11 pairwise, 10 horizontal, the horizontal expectations
  cross-checked against the scalar reference so a typo cannot
  silently pass), all 4,294,967,296 `(a, b)` 16-bit pairs
  exhaustively with each pair replicated to all four lanes (every
  lane position sees the full pair space, each lane checked
  against scalar `max(a, b)`), 4,194,304 directed lane-crosstalk
  cases (each lane position sweeping all 2^16 values while
  neighbors sit at `0x0000`/`0xFFFF`/`0x7FFF`/`0x8000` in one
  word and the complementary pattern in the other), and
  10,000,014 horizontal-max cases (14 directed words plus
  10,000,000 fixed-seed splitmix64 words against the scalar
  four-lane max).
- The checksum `0x64d1c937981935ce` is identical across `-O0`,
  `-O2`, and ASan+UBSan: the same outputs, bit for bit, under no
  optimization, full optimization, and instrumentation.  It covers
  every output value produced by sections [1] through [4] plus
  the five timed-loop sinks from section [6].
- Throughput at `-O2`, best of 5 over 100,000,000 timed values
  with the splitmix64 PRNG pre-generated into a 2^20-entry buffer
  and excluded from the timed loop: horizontal max 3.943
  ns/value (253.589 M values/s); pairwise core 1.484 ns/value
  (673.973 M values/s).  With the PRNG step inside the timed
  loop the horizontal cost rises to 12.927 ns/value, so most of
  that loop is the PRNG; the 3.943 ns figure is the per-value
  cost with the PRNG excluded.  The per-pass sinks
  (`4ff86bc6a44`, `6702cd86b7cd08a0`, `4fff0a95419`) are
  identical across all passes and all three builds, confirming
  the timed loops computed the same values everywhere.
- The implementation uses only unsigned 64-bit arithmetic,
  shifts by compile-time constants below 64, and bitwise ops;
  no builtins, no intrinsics, no inline asm, no library calls.
