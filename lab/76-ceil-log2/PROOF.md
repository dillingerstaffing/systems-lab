<!-- PROOF-HEADER
Checks: 10065744
Mismatches: 0
Checksum: 0x52130d09b192774b
Throughput: 6.047 ns/value at -O2, best of 50
Verdict: PASS
-->
# PROOF.md - lab/76-ceil-log2

`ceil_log2_64(x)` in `ceil_log2.h`: the smallest n with 2^n >= x,
for a 64-bit word, from the fill-cascade floor, the
`(x & (x - 1)) == 0` power-of-two test, and
`ceil = floor + (not power of two ? 1 : 0)`.

Why the construction is exact:

- Fill cascade: each stage `x |= x >> k` (k = 1, 2, 4, 8, 16,
  32) doubles the downward reach of every 1 bit; 1+2+4+8+16+32
  = 63 covers the whole word, so afterward every bit at or
  below the highest set bit of x is 1, and fill only turns 0s
  into 1s at or below an existing 1, so no bit above the top
  set bit changes.  For x >= 1 with top set bit at position f,
  the filled word has exactly bits f..0 set: popcount f + 1.
- SWAR popcount: `z - ((z >> 1) & 0x55..)` leaves pairwise sums
  in disjoint 2-bit fields (each difference is exact); adding
  the 4-bit halves keeps disjoint 4-bit sums; `(z + (z >> 4)) &
  0x0F..` leaves each byte's count in its own byte (byte sums
  are at most 8, no cross-byte carry); multiplying by
  0x0101010101010101 adds all eight byte counts into the top
  byte, recovered by `>> 56`.  Minus 1 gives floor_log2(x) = f.
  All arithmetic is unsigned 64-bit, so every shift, add,
  subtract, and multiply is fully defined.
- Power-of-two test: x - 1 borrows through the trailing zeros
  and clears the lowest set bit; `x & (x - 1)` removes exactly
  that one bit, so it is 0 iff x had exactly one bit set, i.e.
  iff x = 2^f for x >= 1.
- Ceiling sum: if x = 2^f then floor = f and ceil = f; else
  2^f < x < 2^(f+1) and the smallest n with 2^n >= x is f + 1.
  The two cases partition every x >= 1, so the sum is exact.
- x = 0 is outside the differential domain by explicit
  contract: `ceil_log2_64(0)` returns 0 (the guard sits before
  the fill cascade, whose popcount-minus-1 would wrap on the
  empty input).  The contract is pinned by a dedicated test row
  and never enters the differential set.

Scope note: the differential test checks `ceil_log2_64(x)`
against the independent doubling-loop reference on every
exercised word.  The fill, popcount, and power-of-two
identities are argued from the header above; the disassembly
section below confirms the compiler did not replace them with
a count-class instruction.  No timing or quality claims are
made; throughput is reported as measured, nothing more.

## Build log (verbatim)

```
$ make clean && make
rm -f test_ceil_log2_o0 test_ceil_log2_o2 test_ceil_log2_asan
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_ceil_log2_o0 test_ceil_log2.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_ceil_log2_o2 test_ceil_log2.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_ceil_log2_asan test_ceil_log2.c
```

Zero warnings under `-Wall -Wextra -Werror`.  Zero sanitizer
reports on the full 10,065,744-case suite under ASan+UBSan
(exit 0, no sanitizer output).

## Run output, build -O2 (verbatim)

```
ceil_log2_64 differential test, build -O2
[1/5] anchors
  anchors checked: 16, mismatches so far: 0
[2/5] exhaustive 16-bit inputs
  done: cases=65552 mismatches=0
[3/5] boundary rows 2^k, 2^k-1, 2^k+1, k = 0..63
  done: cases=65744 mismatches=0
[4/5] random 64-bit words (splitmix64, seed 0x123456789ABCDEF0)
  done: cases=10065744 mismatches=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput best of 50 passes over 2000000 pre-generated words
  (100000000 total timed values): 6.047 ns/value (165.367 M values/s)
  with PRNG in the timed loop: 10.775 ns/value (sink=782a23d)
total verification cases: 10065744
total mismatches: 0
FNV-1a checksum of all outputs: 0x52130d09b192774b
RESULT: PASS
```

## Run output, builds -O0 and asan+ubsan (summary)

```
ceil_log2_64 differential test, build -O0
total verification cases: 10065744
total mismatches: 0
FNV-1a checksum of all outputs: 0x52130d09b192774b
RESULT: PASS
ceil_log2_64 differential test, build asan+ubsan
total verification cases: 10065744
total mismatches: 0
FNV-1a checksum of all outputs: 0x52130d09b192774b
RESULT: PASS
```

The FNV-1a checksum over every output count is byte-identical
across `-O0`, `-O2`, and ASan+UBSan: `0x52130d09b192774b`.

## Disassembly check at -O2 (gcc 13.3.0)

```
$ objdump -d test_ceil_log2_o2 | grep -ciE '\b(bsr|lzcnt|tzcnt|popcnt)\b'
0
```

Zero `bsr`/`lzcnt`/`tzcnt`/`popcnt` instructions anywhere in the
binary.  The SWAR mask survives as an immediate
(`movabs $0x5555555555555555,...`) and the hot path is plain
`shr`/`or`/`and`/`lea`/`imul`: the fill cascade and the SWAR
popcount were not folded into a count-class instruction.

## Throughput at -O2

Best of 50 passes over 2,000,000 pre-generated splitmix64 words
(100,000,000 total timed values, PRNG excluded from timing):
6.047 ns/value (165.367 M values/s).  With the splitmix64 PRNG
inside the timed loop: 10.775 ns/value.  (The -O0 build measured
19.616 ns/value and the ASan+UBSan build 6.988 ns/value over the
same 100M timed values.)
