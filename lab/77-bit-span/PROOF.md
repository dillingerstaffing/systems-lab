<!-- PROOF-HEADER
Checks: 10065685
Mismatches: 0
Checksum: 0x7c36594632d8db46
Throughput: 7.210 ns/value at -O2, best of 5
Verdict: PASS
-->
# PROOF.md: lab/77-bit-span

Genuine build log and run output, captured 2026-09-10.  Toolchain:
gcc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0.  Flags:
`-std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L`.

## Build log

```
$ make all
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_bit_span_o0 test_bit_span.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_bit_span_o2 test_bit_span.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
		-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o $@ test_bit_span.c
```

No warnings, no errors (`-Werror` was in force for all three builds).

## Run output, -O2 build (exit 0)

```
bit_span differential test, build -O2
[1/6] de Bruijn table self-check + anchors
  table: 64/64 entries match the derived rule
  anchors checked: 11, mismatches so far: 0
[2/6] exhaustive 16-bit inputs
  done: cases=65547 mismatches=0
[3/6] directed edge words
  done: cases=65685 mismatches=0
[4/6] random 64-bit words (splitmix64, seed 0x123456789ABCDEF0)
  done: cases=10065685 mismatches=0
[5/6] cross-build checksum: compare the FNV-1a line across -O0, -O2, ASan+UBSan
[6/6] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 7.415 ns/value (134.853 M values/s) sink=16b96ec9c
  throughput pass 1: 7.522 ns/value (132.948 M values/s) sink=16b96ec9c
  throughput pass 2: 7.210 ns/value (138.699 M values/s) sink=16b96ec9c
  throughput pass 3: 7.673 ns/value (130.332 M values/s) sink=16b96ec9c
  throughput pass 4: 9.897 ns/value (101.039 M values/s) sink=16b96ec9c
  throughput best of 5: 7.210 ns/value (138.699 M values/s)
  with PRNG in the timed loop: 10.567 ns/value (sink=245bcee5)
total verification cases: 10065685
total mismatches: 0
FNV-1a checksum of all outputs: 0x7c36594632d8db46
RESULT: PASS
```

## Run summary, -O0 and ASan+UBSan builds (both exit 0)

-O0 build: 10,065,685 cases, 0 mismatches, checksum
`0x7c36594632d8db46`, RESULT: PASS.  Throughput best of 5:
21.171 ns/value.

ASan+UBSan build: 10,065,685 cases, 0 mismatches, checksum
`0x7c36594632d8db46`, RESULT: PASS.  Zero sanitizer reports.

## What the numbers mean

- The checksum `0x7c36594632d8db46` is identical across `-O0`,
  `-O2`, and ASan+UBSan: the same 10,065,685 outputs, bit for bit,
  under no optimization, full optimization, and instrumentation.
- The differential test covers all 65,536 16-bit inputs, every
  single-bit position, an exact witness of every possible span value
  0..63, and 10,000,000 fixed-seed 64-bit words, against two
  independent naive loop references.  Zero mismatches.
- The de Bruijn table was verified against the independently derived
  rule, 64/64 entries.
- The `x = 0` contract row (`bit_span(0) = 0`) was pinned by the
  hand-checked anchor and included in the exhaustive pass.
- Throughput at -O2 over 100M timed values: 7.210 ns/value best of 5.
  With the splitmix64 PRNG step included the loop costs 10.567
  ns/value, so the PRNG step is a meaningful share of the timed loop;
  the 7.210 ns figure excludes it.
