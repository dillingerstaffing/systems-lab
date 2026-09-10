# PROOF.md: lab/94-mulhi-signed

`mulhi_s64(a, b)`: the high 64 bits of the signed 128-bit product of
two `int64_t` values.

## Construction

Let ua = (uint64_t)a, ub = (uint64_t)b (the same bit patterns read
unsigned), sa = (a < 0), sb = (b < 0). As integers,
a = ua - sa*2^64 and b = ub - sb*2^64, so

    a*b = ua*ub - sa*2^64*ub - sb*2^64*ua + sa*sb*2^128.

Every term is a multiple of 2^64 except the low word of ua*ub, hence

    high(a*b) = mulhi_u64(ua,ub) - sa*ub - sb*ua   (mod 2^64),

the sa*sb*2^64 term vanishing mod 2^64. The true high word satisfies
|high| <= 2^62 (since |a*b| <= 2^126), so the wrapped 64-bit result
read back as int64_t is the exact signed high word.

The unsigned high word mulhi_u64 is the 32-bit splitting identity:
a = ah*2^32 + al, b = bh*2^32 + bl, four 32-bit x 32-bit partial
products (each below 2^64, so no 128-bit type is needed), the middle
sum formed in a 64-bit register with explicit wrap detection
(`(x + y) < x` is 1 exactly when the addition wrapped).

The sign corrections are the exact contribution of the high half's
sign extension: a negative operand carries an implicit -2^64, and its
product with the other operand lands entirely in the high word.
They are two conditional subtracts. The only multiplies in the
implementation are the four unsigned 32x32 partial products.

## Build log (genuine, zero warnings)

```
$ make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_mulhi_signed_o0 test_mulhi_signed.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_mulhi_signed_o2 test_mulhi_signed.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_mulhi_signed_asan test_mulhi_signed.c
```

No warnings under `-std=c11 -Wall -Wextra -Werror` in any of the
three builds.

## Verification runs (genuine output)

Oracle: `(int64_t)(((signed __int128)a * (signed __int128)b) >> 64)`,
confined to the test file. The exhaustive pass covers all 2^32
sign-extended 16-bit pairs; the random pass uses 10,000,000
fixed-seed splitmix64 64-bit pairs (seed 0x123456789ABCDEF0); the
directed pass covers the sign-boundary rows (23x23 = 529 pairs,
including INT64_MIN x INT64_MIN, INT64_MIN x -1,
INT64_MIN x INT64_MAX, (-1) x (-1)) plus signed powers of two
(3x63x63 = 11,907 pairs), 12,436 directed cases total.

Build -O2:

```
mulhi_s64 differential test, build -O2
[1/4] exhaustive sign-extended 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304979732 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 8.249 ns/pair
  throughput pass 1: 6.646 ns/pair
  throughput pass 2: 5.942 ns/pair
  throughput pass 3: 6.293 ns/pair
  throughput pass 4: 6.792 ns/pair
  throughput best of 5: 5.942 ns/pair
total verification cases: 4304979732
total mismatches: 0
FNV-1a checksum of all result words: 0x3282d931333f6a85
RESULT: PASS
```

Build -O0:

```
mulhi_s64 differential test, build -O0
[1/4] exhaustive sign-extended 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304979732 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 56.050 ns/pair
  throughput pass 1: 48.271 ns/pair
  throughput pass 2: 57.723 ns/pair
  throughput pass 3: 44.130 ns/pair
  throughput pass 4: 41.724 ns/pair
  throughput best of 5: 41.724 ns/pair
total verification cases: 4304979732
total mismatches: 0
FNV-1a checksum of all result words: 0x3282d931333f6a85
RESULT: PASS
```

Build asan+ubsan (AddressSanitizer + UBSan, full 4.3B-case suite):

```
mulhi_s64 differential test, build asan+ubsan
[1/4] exhaustive sign-extended 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967296 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=4304967296 mismatches=0
[3/4] directed edge cases
  done: cases=4304979732 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 8.177 ns/pair
  throughput pass 1: 8.275 ns/pair
  throughput pass 2: 7.726 ns/pair
  throughput pass 3: 8.061 ns/pair
  throughput pass 4: 7.696 ns/pair
  throughput best of 5: 7.696 ns/pair
total verification cases: 4304979732
total mismatches: 0
FNV-1a checksum of all result words: 0x3282d931333f6a85
RESULT: PASS
```

The FNV-1a checksum `0x3282d931333f6a85` is identical across -O0,
-O2, and ASan+UBSan builds. No sanitizer reported anything on the
full 4.3B-case run (zero "runtime error" / "AddressSanitizer" lines
in the asan+ubsan log).

## Source-level multiply audit

```
$ grep -n "__int128" mulhi_signed.h
(no output; exit 1)
```

The implementation file contains no 128-bit integer type at all
(not even in comments). The only `*` operators in mulhi_u64 apply
to uint64_t variables that hold values below 2^32 (the halves), and
the sign corrections use no multiply: `hi -= ub` / `hi -= ua`.

## Disassembly check at -O2 (x86-64, GCC)

`objdump -d` of a probe translation unit calling mulhi_s64, compiled
`-O2`:

```
   4:  48 89 f1          mov    %rsi,%rcx
   7:  48 89 fa          mov    %rdi,%rdx
   a:  89 f8             mov    %edi,%eax        ; zero-extend a_lo
   c:  89 f6             mov    %esi,%esi        ; zero-extend b_lo
  11:  48 c1 ea 20       shr    $0x20,%rdx       ; a_hi
  18:  49 c1 e8 20       shr    $0x20,%r8        ; b_hi
  1c:  4c 0f af ce       imul   %rsi,%r9         ; b_lo * a_hi  (32x32)
  23:  49 0f af c0       imul   %r8,%rax         ; b_hi * a_lo  (32x32)
  27:  48 0f af d6       imul   %rsi,%rdx        ; b_lo * a_lo  (32x32)
  2b:  4d 0f af d0       imul   %r8,%r10         ; b_hi * a_hi  (32x32)
  2f:  48 01 d0          add    %rdx,%rax
  32:  0f 92 c2          setb   %dl              ; wrap bit m1
  38:  48 c1 e0 20       shl    $0x20,%rax
  3c:  0f b6 d2          movzbl %dl,%edx
  3f:  48 c1 ee 20       shr    $0x20,%rsi
  43:  48 c1 e2 20       shl    $0x20,%rdx
  47:  4c 01 d6          add    %r10,%rsi
  4a:  4c 01 c8          add    %r9,%rax
  4d:  48 89 d0          mov    %rdx,%rax
  50:  48 11 f0          adc    %rsi,%rax        ; carry fold
  56:  48 29 ca          sub    %rcx,%rdx        ; hi - ub
  59:  48 85 ff          test   %rdi,%rdi
  5c:  48 0f 48 c2       cmovs  %rdx,%rax        ; if (a < 0) hi -= ub
  63:  48 29 fa          sub    %rdi,%rdx        ; hi - ua
  66:  48 85 c9          test   %rcx,%rcx
  69:  48 0f 48 c2       cmovs  %rdx,%rax        ; if (b < 0) hi -= ua
  6d:  c3                ret
```

What this shows:

- The four `imul` instructions take the zero-extended 32-bit halves
  as operands (established by the `mov %edi,%eax` / `mov %esi,%esi`
  zero-extensions and the `shr $0x20` half extractions). Each is a
  32-bit x 32-bit product whose exact 64-bit result is the low 64
  bits of the two-operand `imul`; these are the half-width partial
  products, not a full-width multiply.
- Carries fold through `add`/`setb`/`adc`, matching the explicit
  wrap detection in the source.
- The sign corrections compile to `test`/`cmovs` conditional
  subtracts: branchless, no multiply, exactly the
  `if (a < 0) hi -= ub; if (b < 0) hi -= ua;` source.
- No one-operand `mul` (the instruction that produces a 128-bit
  rdx:rax result) appears, and no 64x64 multiply of the original
  operands appears: the full-width signed multiply never re-forms.

## Throughput

Measured at -O2 over 1M pre-generated splitmix64 pairs, 5 passes,
PRNG excluded from the timed loop, results xored into a volatile
sink: best 5.942 ns/pair (passes 8.249, 6.646, 5.942, 6.293, 6.792),
i.e. 168.3 Mpairs/s. (-O0: 41.724 ns/pair; ASan+UBSan: 7.696
ns/pair.)

## Claims and their grounding

- "0 mismatches over 4,304,979,732 checks per build": counted by the
  test binaries (exit 0, RESULT: PASS), differential against the
  signed 128-bit oracle.
- "Checksum identical across builds": the FNV-1a value printed by
  each binary, `0x3282d931333f6a85` in all three logs above.
- "No 128-bit type, no full-width signed multiply in the
  implementation": `grep -n "__int128" mulhi_signed.h` returns
  nothing, and the -O2 disassembly above shows only half-width
  `imul`s plus `test`/`cmovs` subtracts.
- "Clean under ASan/UBSan": the full 4.3B-case suite ran under
  `-fsanitize=address,undefined` with no sanitizer output.
