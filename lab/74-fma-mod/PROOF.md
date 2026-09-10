# PROOF.md - lab/74-fma-mod

`fma_mod64(a, b, c)` in `fma_mod.h`: the exact value `(a * b + c) mod
2^64`, built only from the 32-bit splitting identities.

Why the construction is exact:

- Split `a = ah * 2^32 + al`, `b = bh * 2^32 + bl` with each half
  below 2^32.  Then `a * b = p3 * 2^64 + (p1 + p2) * 2^32 + p0` with
  `p0 = al*bl`, `p1 = al*bh`, `p2 = ah*bl`, `p3 = ah*bh`.  Each
  partial product has both factors below 2^32, so each is exact in
  64 bits.
- Middle column `col1 = p1 + p2 + (p0 >> 32)` is an integer below
  2^65 (since `p1 + p2 <= 2 * (2^32 - 1)^2` and `(p0 >> 32) < 2^32`).
  With `mid = p1 + p2`, `k1 = (mid < p1)`, `mid2 = mid + (p0 >> 32)`,
  `k2 = (mid2 < (p0 >> 32))`, the identity
  `col1 = (k1 + k2) * 2^64 + mid2` holds exactly, because `(x + y) < x`
  is 1 exactly when the unsigned addition wrapped.  (`k1 + k2 <= 1`:
  both bits set would need `col1 >= 2^65`.)
- Modulo 2^64, `p3 * 2^64` vanishes and
  `col1 * 2^32 = (k1 + k2) * 2^96 + mid2 * 2^32` reduces to
  `((uint32_t)mid2) * 2^32`, since `(k1 + k2) * 2^96` is a multiple
  of 2^64 and `mid2 * 2^32 mod 2^64 = (mid2 mod 2^32) * 2^32`.
  Hence `(a * b) mod 2^64` is exactly
  `(uint32_t)p0 + (((uint32_t)mid2) << 32)`, a sum below 2^64
  (`(uint32_t)p0 < 2^32` and `((uint32_t)mid2 << 32) <= 2^64 - 2^32`),
  so it is formed with no wrap possible.
- The high word `ab_hi = p3 + (k1 + k2) * 2^32 + (mid2 >> 32)` equals
  `floor(a * b / 2^64)` by the column identity; since `a * b < 2^128`,
  `ab_hi < 2^64`, and every partial sum is a sub-sum of non-negative
  terms bounded by `ab_hi`, no addition in it can wrap.  It is
  discarded (`(void)ab_hi`), since `mod 2^64` keeps only the low word.
- `return ab_lo + c`: unsigned 64-bit addition wraps, and wraparound
  is exactly addition mod 2^64, so the result is `(a * b + c) mod
  2^64`.  No `__int128`, no intrinsics, no builtins anywhere in the
  implementation.

Scope note: the differential test verifies the returned low word
against the `unsigned __int128` oracle on every exercised triple.
The high word `ab_hi` is computed to keep the accumulation
reviewable; its exactness is established by the column identity in
the header comments, not by a differential check.  The exhaustive
16-bit sweep has `ah = bh = 0`, so it covers the `p0` partial-product
path and the final `+ c` exhaustively but never sets the carries;
the carry paths are covered by the directed set (both `k1` and `k2`
forced by constructed triples, confirmed by the independent tally)
and by 725,371 `k1` events over the 10M random triples.  `k2` fired
0 times over the random triples: it needs `mid` within 2^32 of the
2^64 boundary, which is rare under random 64-bit inputs, so it is
covered by the two constructed directed triples instead.  No timing
or quality claims are made; throughput is reported as measured,
nothing more.

## Build log (verbatim)

```
$ make clean && make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_fma_mod_o0 test_fma_mod.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_fma_mod_o2 test_fma_mod.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_fma_mod_asan test_fma_mod.c
```

Zero warnings under `-Wall -Wextra -Werror`.  Zero sanitizer reports
on the full 34,369,738,593-case suite under ASan+UBSan.

## Run output, build -O2 (verbatim)

```
fma_mod64 differential test, build -O2
[1/5] anchors
  anchors checked: 7, mismatches so far: 0
[2/5] exhaustive 16-bit (a,b) pairs x 8 c values
  done: cases=34359738375 mismatches=0
[3/5] directed edge triples
  done: cases=34359738593 mismatches=0
  carry tally over directed triples: k1 events=4 k2 events=2
[4/5] random 64-bit triples (splitmix64, seed 0xF1A074F74A04D0D)
  done: cases=34369738593 mismatches=0
  carry tally over random triples: k1 events=725371 k2 events=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 2.423 ns/triple (412.745 M triples/s) sink=898aecc27521474d
  throughput pass 1: 2.303 ns/triple (434.158 M triples/s) sink=898aecc27521474d
  throughput pass 2: 2.152 ns/triple (464.637 M triples/s) sink=898aecc27521474d
  throughput pass 3: 2.155 ns/triple (464.049 M triples/s) sink=898aecc27521474d
  throughput pass 4: 2.144 ns/triple (466.320 M triples/s) sink=898aecc27521474d
  throughput best of 5: 2.144 ns/triple (466.320 M triples/s)
total verification cases: 34369738593
total mismatches: 0
FNV-1a checksum of all outputs: 0xee898af0ca125a96
RESULT: PASS
```

## Run output, build -O0 (verbatim)

```
fma_mod64 differential test, build -O0
[1/5] anchors
  anchors checked: 7, mismatches so far: 0
[2/5] exhaustive 16-bit (a,b) pairs x 8 c values
  done: cases=34359738375 mismatches=0
[3/5] directed edge triples
  done: cases=34359738593 mismatches=0
  carry tally over directed triples: k1 events=4 k2 events=2
[4/5] random 64-bit triples (splitmix64, seed 0xF1A074F74A04D0D)
  done: cases=34369738593 mismatches=0
  carry tally over random triples: k1 events=725371 k2 events=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 11.421 ns/triple (87.559 M triples/s) sink=898aecc27521474d
  throughput pass 1: 11.673 ns/triple (85.666 M triples/s) sink=898aecc27521474d
  throughput pass 2: 11.396 ns/triple (87.748 M triples/s) sink=898aecc27521474d
  throughput pass 3: 11.767 ns/triple (84.980 M triples/s) sink=898aecc27521474d
  throughput pass 4: 11.363 ns/triple (88.007 M triples/s) sink=898aecc27521474d
  throughput best of 5: 11.363 ns/triple (88.007 M triples/s)
total verification cases: 34369738593
total mismatches: 0
FNV-1a checksum of all outputs: 0xee898af0ca125a96
RESULT: PASS
```

## Run output, build asan+ubsan (verbatim)

```
fma_mod64 differential test, build asan+ubsan
[1/5] anchors
  anchors checked: 7, mismatches so far: 0
[2/5] exhaustive 16-bit (a,b) pairs x 8 c values
  done: cases=34359738375 mismatches=0
[3/5] directed edge triples
  done: cases=34359738593 mismatches=0
  carry tally over directed triples: k1 events=4 k2 events=2
[4/5] random 64-bit triples (splitmix64, seed 0xF1A074F74A04D0D)
  done: cases=34369738593 mismatches=0
  carry tally over random triples: k1 events=725371 k2 events=0
[5/5] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 5.464 ns/triple (183.023 M triples/s) sink=898aecc27521474d
  throughput pass 1: 5.363 ns/triple (186.454 M triples/s) sink=898aecc27521474d
  throughput pass 2: 5.398 ns/triple (185.251 M triples/s) sink=898aecc27521474d
  throughput pass 3: 5.277 ns/triple (189.503 M triples/s) sink=898aecc27521474d
  throughput pass 4: 5.499 ns/triple (181.854 M triples/s) sink=898aecc27521474d
  throughput best of 5: 5.277 ns/triple (189.503 M triples/s)
total verification cases: 34369738593
total mismatches: 0
FNV-1a checksum of all outputs: 0xee898af0ca125a96
RESULT: PASS
```

The FNV-1a checksum `0xee898af0ca125a96` is identical across all
three builds.

## Disassembly check (build -O2)

A probe translation unit wrapping `fma_mod64` in a `noinline`
function was compiled with `-O2` and disassembled with `objdump -d`:

```
0000000000000000 <fma_mod64_probe>:
   4:	89 f9                	mov    %edi,%ecx
   6:	89 f0                	mov    %esi,%eax
   b:	48 c1 ee 20          	shr    $0x20,%rsi
  12:	48 c1 ef 20          	shr    $0x20,%rdi
  16:	48 0f af f1          	imul   %rcx,%rsi      ; p1 = al*bh
  1a:	48 0f af d0          	imul   %rax,%rdx      ; p0 = al*bl
  1e:	48 0f af f8          	imul   %rax,%rdi      ; p2 = ah*bl
  27:	48 8d 04 3e          	lea    (%rsi,%rdi,1),%rax ; mid = p1+p2
  2b:	48 c1 e9 20          	shr    $0x20,%rcx      ; p0 >> 32
  2f:	48 01 c8             	add    %rcx,%rax       ; mid2 = mid + (p0>>32)
  32:	48 c1 e0 20          	shl    $0x20,%rax      ; (mid2 mod 2^32) << 32
  36:	48 09 d0             	or     %rdx,%rax       ; + (p0 mod 2^32)
  39:	4c 01 c0             	add    %r8,%rax        ; + c
  3c:	c3                   	ret
```

The 32-bit splitting survives: the halves are extracted with
zero-extending moves and `shr $0x20`, three separate `imul`
partial products are formed (`p0`, `p1`, `p2`), and the result is
accumulated with shifts and adds.  It is not folded into a single
64-bit multiply of the original `a` and `b`.  The carry-bit
comparisons `k1`/`k2` and the high word `ab_hi` (hence `p3`) are
eliminated as dead code, which is legitimate: they feed only the
high 64 bits that `mod 2^64` discards, and they remain in the source
so every addition's exactness stays reviewable.

## Throughput methodology

2,000,000 triples are generated once with splitmix64 before timing
starts, so the PRNG is not part of the measured loop.  Each pass
xors every `fma_mod64(ta[i], tb[i], tc[i])` into a `volatile` sink.
Best of 5 passes at `-O2`: 2.144 ns/triple (466.320 M triples/s).
This measures the timed loop as written (streaming 24 bytes per
triple through the memory hierarchy), not the function in isolation.
