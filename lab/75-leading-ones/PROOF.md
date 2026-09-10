<!-- PROOF-HEADER
Checks: 10065619
Mismatches: 0
Checksum: 0x89c5b240295bdfcc
Throughput: 5.483 ns/value at -O2, best of 5
Verdict: PASS
-->
# PROOF.md - lab/75-leading-ones

`clo64(x)` in `leading_ones.h`: count of consecutive 1 bits from the
most significant bit, from the invert-then-scan identity `clo(x) =
clz(~x)` with clz built from the shift/OR fill cascade.

Why the construction is exact:

- Inverting maps leading ones of `x` onto leading zeros of `~x` one
  for one, in the same positions: bit `i` of `x` is 1 iff bit `i` of
  `~x` is 0.  So `clo(x) = clz(~x)` by the definition of both
  counts.
- Fill cascade: for `y |= y >> k` with k = 1, 2, 4, 8, 16, 32, each
  stage doubles the reach of downward propagation, so after all six
  stages every bit at or below the highest set bit of `~x` is 1.
  (Induction: after stage k the fill extends 2k below each 1 bit;
  1+2+4+8+16+32 = 63 reaches the bottom bit from any position.)
- If the top set bit of `~x` is at position p (63 = most
  significant), the filled value has bits 63..p all set, and fill
  only sets bits at or below an existing 1, so bits above p stay 0.
  Hence `~y` has ones exactly in positions 63..p, a count of
  64 - p = clz(~x).
- The degenerate input `~x = 0` (x = 0xFFFFFFFFFFFFFFFF) fills to 0,
  and the popcount of `~0` is 64, so the zero-input contract
  returns 64 with no special case.  `x = 0` itself has zero leading
  ones and returns 0.
- The SWAR popcount: `z - ((z >> 1) & 0x55..)` leaves pairwise sums
  in disjoint 2-bit fields (each difference is exact because the
  subtrahend is half the field); adding the 4-bit halves keeps
  disjoint 4-bit sums; `(z + (z >> 4)) & 0x0F..` leaves each byte's
  count in its own byte (byte sums are at most 8, no cross-byte
  carry); multiplying by 0x0101010101010101 adds all eight byte
  counts into the top byte, recovered by `>> 56`.  All arithmetic
  is on unsigned 64-bit values, so every shift, add, subtract, and
  multiply is fully defined.

Scope note: the differential test checks `clo64(x)` against the
naive bit-scan reference on every exercised word.  The fill and
popcount identities are argued from the header above; the
disassembly section below confirms the compiler did not replace
them with a count-class instruction.  No timing or quality claims
are made; throughput is reported as measured, nothing more.

## Build log (verbatim)

```
$ make clean && make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_leading_ones_o0 test_leading_ones.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_leading_ones_o2 test_leading_ones.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_leading_ones_asan test_leading_ones.c
```

Zero warnings under `-Wall -Wextra -Werror`.  Zero sanitizer reports
on the full 10,065,619-case suite under ASan+UBSan.

## Run output, build -O2 (verbatim)

```
clo64 differential test, build -O2
[1/5] anchors
  anchors checked: 10, mismatches so far: 0
[2/5] exhaustive 16-bit inputs
  done: cases=65546 mismatches=0
[3/5] directed edge words
  done: cases=65619 mismatches=0
[4/5] random 64-bit words (splitmix64, seed 0xC10DCAFE12345678)
  done: cases=10065619 mismatches=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 5.774 ns/value (173.182 M values/s) sink=1e8ce9
  throughput pass 1: 5.613 ns/value (178.156 M values/s) sink=1e8ce9
  throughput pass 2: 5.558 ns/value (179.912 M values/s) sink=1e8ce9
  throughput pass 3: 5.529 ns/value (180.864 M values/s) sink=1e8ce9
  throughput pass 4: 5.483 ns/value (182.386 M values/s) sink=1e8ce9
  throughput best of 5: 5.483 ns/value (182.386 M values/s)
  with PRNG in the timed loop: 8.267 ns/value (sink=1e7d3d)
total verification cases: 10065619
total mismatches: 0
FNV-1a checksum of all outputs: 0x89c5b240295bdfcc
RESULT: PASS
```

## Run output, build -O0 (verbatim)

```
clo64 differential test, build -O0
[1/5] anchors
  anchors checked: 10, mismatches so far: 0
[2/5] exhaustive 16-bit inputs
  done: cases=65546 mismatches=0
[3/5] directed edge words
  done: cases=65619 mismatches=0
[4/5] random 64-bit words (splitmix64, seed 0xC10DCAFE12345678)
  done: cases=10065619 mismatches=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 12.715 ns/value (78.648 M values/s) sink=1e8ce9
  throughput pass 1: 13.425 ns/value (74.487 M values/s) sink=1e8ce9
  throughput pass 2: 12.797 ns/value (78.145 M values/s) sink=1e8ce9
  throughput pass 3: 13.508 ns/value (74.029 M values/s) sink=1e8ce9
  throughput pass 4: 12.988 ns/value (76.994 M values/s) sink=1e8ce9
  throughput best of 5: 12.715 ns/value (78.648 M values/s)
  with PRNG in the timed loop: 29.171 ns/value (sink=1e7d3d)
total verification cases: 10065619
total mismatches: 0
FNV-1a checksum of all outputs: 0x89c5b240295bdfcc
RESULT: PASS
```

## Run output, build asan+ubsan (verbatim)

```
clo64 differential test, build asan+ubsan
[1/5] anchors
  anchors checked: 10, mismatches so far: 0
[2/5] exhaustive 16-bit inputs
  done: cases=65546 mismatches=0
[3/5] directed edge words
  done: cases=65619 mismatches=0
[4/5] random 64-bit words (splitmix64, seed 0xC10DCAFE12345678)
  done: cases=10065619 mismatches=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 12.509 ns/value (79.944 M values/s) sink=1e8ce9
  throughput pass 1: 10.394 ns/value (96.208 M values/s) sink=1e8ce9
  throughput pass 2: 6.952 ns/value (143.842 M values/s) sink=1e8ce9
  throughput pass 3: 12.663 ns/value (78.970 M values/s) sink=1e8ce9
  throughput pass 4: 10.679 ns/value (93.640 M values/s) sink=1e8ce9
  throughput best of 5: 6.952 ns/value (143.842 M values/s)
  with PRNG in the timed loop: 8.568 ns/value (sink=1e7d3d)
total verification cases: 10065619
total mismatches: 0
FNV-1a checksum of all outputs: 0x89c5b240295bdfcc
RESULT: PASS
```

The FNV-1a checksum `0x89c5b240295bdfcc` is identical across all
three builds.

## Disassembly check (build -O2)

A probe translation unit wrapping `clo64` in a `noinline` function
was compiled with `-O2` and disassembled with `objdump -d`:

```
0000000000000000 <clo64_probe>:
   0:	endbr64
   4:	movabs $0x5555555555555555,%rcx
   e:	mov    %rdi,%rax
  11:	not    %rax                       ; ~x
  14:	mov    %rax,%rdx
  17:	shr    $1,%rdx
  1a:	or     %rdx,%rax                  ; y |= y >> 1
  1d:	mov    %rax,%rdx
  20:	shr    $0x2,%rdx
  24:	or     %rdx,%rax                  ; y |= y >> 2
  27:	mov    %rax,%rdx
  2a:	shr    $0x4,%rdx
  2e:	or     %rdx,%rax                  ; y |= y >> 4
  31:	mov    %rax,%rdx
  34:	shr    $0x8,%rdx
  38:	or     %rdx,%rax                  ; y |= y >> 8
  3b:	mov    %rax,%rdx
  3e:	shr    $0x10,%rdx
  42:	or     %rdx,%rax                  ; y |= y >> 16
  45:	mov    %rax,%rdx
  48:	shr    $0x20,%rdx
  4c:	or     %rdx,%rax                  ; y |= y >> 32
  4f:	not    %rax                       ; ~y
  52:	mov    %rax,%rdx
  55:	shr    $1,%rdx
  58:	and    %rcx,%rdx
  5b:	movabs $0x3333333333333333,%rcx
  65:	sub    %rdx,%rax                  ; pairwise sums
  68:	mov    %rax,%rdx
  6b:	shr    $0x2,%rax
  6f:	and    %rcx,%rax
  72:	and    %rcx,%rdx
  75:	add    %rax,%rdx                  ; nibble sums
  78:	mov    %rdx,%rax
  7b:	shr    $0x4,%rax
  7f:	add    %rdx,%rax
  82:	movabs $0x0f0f0f0f0f0f0f0f,%rdx   ; byte sums (raw bytes
  8c:	and    %rdx,%rax                     verified: 0x0f0f0f0f0f0f0f0f)
  8f:	movabs $0x101010101010101,%rdx
  99:	imul   %rdx,%rax                  ; sum bytes into top byte
  9d:	shr    $0x38,%rax
  a1:	ret
```

No `bsr`, `lzcnt`, or `popcnt` anywhere.  The fill cascade survives
verbatim as six `shr`/`or` pairs, and the SWAR popcount survives as
shifts, masks, adds, and one `imul` for the 0x0101010101010101
byte-sum step: gcc 13.3.0 does not fold either part into a
count-class instruction.  (objdump's text line for the 0x0F0F mask
printed `0xf0f0f0f0f0f0f0f`; the raw instruction bytes read from
the object file are `48 ba 0f 0f 0f 0f 0f 0f 0f 0f`, i.e. the exact
source constant `0x0f0f0f0f0f0f0f0f`, so no folding happened.)

## Throughput methodology

2,000,000 words are generated once with splitmix64 before timing
starts, so the PRNG is not part of the measured loop.  Each pass
adds every `clo64(t[i])` into a `volatile` sink.  Best of 5 passes
at `-O2`: 5.483 ns/value (182.386 M values/s).  A second loop with
the splitmix64 PRNG inside the timed loop measured 8.267 ns/value,
so the PRNG cost in that loop is roughly the 2.8 ns/value
difference.  This measures the timed loop as written (streaming 8
bytes per value through the memory hierarchy), not the function in
isolation.
