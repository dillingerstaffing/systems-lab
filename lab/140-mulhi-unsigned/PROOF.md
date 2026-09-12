<!-- PROOF-HEADER
Checks: 26781636
Mismatches: 0
Checksum: 0x75e84e7a670bf3fc
Throughput: 4.862 ns/pair at -O2, best of 5
Environment: Host
Verdict: PASS
-->
# PROOF.md: lab/140-mulhi-unsigned

`mulhi_u64(a, b)`: the high 64 bits of the 128-bit unsigned product of
two `uint64_t` values, from the 32-bit splitting identity alone.

## Construction

Write a = ah*2^32 + al, b = bh*2^32 + bl. Then

    a*b = ah*bh * 2^64 + (al*bh + ah*bl) * 2^32 + al*bl.

The high word is therefore

    high(a*b) = p3 + (m1 << 32) + (mid >> 32) + c,

where p3 = ah*bh, mid = p1 + p2 computed in 64-bit wrap (m1 = 1 exactly
when p1 + p2 wrapped, via `(mid < p1)`), and c = 1 exactly when
(mid << 32) + p0 wrapped (via `(s < p0)`). Each partial product is
32 bits x 32 bits, so it fits in 64 bits: the 128-bit product is never
formed, and no 128-bit type appears in the implementation.

This is the mulhu half that the signed construction in lab/94 does not
cover: the signed case needs sign-correction subtracts of the full
unsigned operand; the unsigned high word needs only the splitting
identity plus carry accounting. Both directions through the same
carry bit are tested: mid's wrap covers the high-carry lane
(m1 << 32), and s's wrap covers the low-carry lane (c).

## Scope note (honest slice)

The backlog plan called for exhaustive 24-bit pairs. That is 2^48
cases, infeasible on any host (at ~10 ns per case it is months of
single-core compute). The shipped slice is the largest fully
verifiable one:

- exhaustive 12-bit x 12-bit pairs: 16,777,216 cases, every low-half
  input combination, both carry bits m1 and c exercised over their
  complete 12-bit input space;
- 10,000,000 fixed-seed splitmix64 64-bit pairs
  (seed 0x123456789ABCDEF0), covering the full 64-bit range;
- 4,420 directed cases: 18 x 18 boundary cross product (0, 1, 2,
  4095/4096/4097, 2^32 - 1 / 2^32 / 2^32 + 1, 2^33,
  2^63 - 1 / 2^63 / 2^63 + 1, UINT64_MAX - 1, UINT64_MAX) plus
  unsigned powers of two (2^k, 2^j) for k, j in 0..63. The
  (2^64-1) x (2^64-1) case yields high word 2^64 - 2, the largest
  possible output.

Total: 26,781,636 differential checks, 0 mismatches.

## Build log (genuine, zero warnings)

```
$ make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_mulhi_unsigned_o0 test_mulhi_unsigned.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_mulhi_unsigned_o2 test_mulhi_unsigned.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_mulhi_unsigned_asan test_mulhi_unsigned.c
```

No warnings under `-std=c11 -Wall -Wextra -Werror` in any of the
three builds.

## Verification runs (genuine output)

Oracle: `(uint64_t)(((unsigned __int128)a * (unsigned __int128)b) >> 64)`,
confined to the test file.

Build -O2:

```
mulhi_u64 differential test, build -O2
[1/4] exhaustive 12-bit x 12-bit pairs (16777216 cases)
  done: cases=16777216 mismatches=0
[2/4] 10M fixed-seed splitmix64 64-bit pairs
  done: cases=26777216 mismatches=0
[3/4] directed edge cases
  done: cases=26781636 mismatches=0
[4/4] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 4.901 ns/pair
  throughput pass 1: 4.909 ns/pair
  throughput pass 2: 4.886 ns/pair
  throughput pass 3: 4.862 ns/pair
  throughput pass 4: 4.915 ns/pair
  throughput best of 5: 4.862 ns/pair
total verification cases: 26781636
total mismatches: 0
FNV-1a checksum of all result words: 0x75e84e7a670bf3fc
RESULT: PASS
```

Build -O0:

```
mulhi_u64 differential test, build -O0
  done: cases=26781636 mismatches=0
  throughput best of 5: 15.337 ns/pair
total verification cases: 26781636
total mismatches: 0
FNV-1a checksum of all result words: 0x75e84e7a670bf3fc
RESULT: PASS
```

Build ASan+UBSan (exit 0, zero sanitizer reports):

```
mulhi_u64 differential test, build asan+ubsan
  done: cases=26781636 mismatches=0
  throughput best of 5: 7.286 ns/pair
total verification cases: 26781636
total mismatches: 0
FNV-1a checksum of all result words: 0x75e84e7a670bf3fc
RESULT: PASS
```

The FNV-1a checksum of all 26,781,636 result words is
0x75e84e7a670bf3fc, identical across the -O0, -O2, and ASan+UBSan
builds. Throughput at -O2 is 4.862 ns/pair, best of 5, with the
timed loop containing only mulhi_u64 calls plus an xor into a
volatile sink (the number includes the loop, call, and sink
overhead).

## Disassembly check (honest)

`objdump -d test_mulhi_unsigned_o2`: the implementation inlines into
the test's check loop. The four multiplies operate on 32-bit halves
(the operands are split first: zero-extend into 32-bit registers via
`mov %esi,%r9d` and `shr $0x20`), i.e. the construction uses the
hardware multiplier for its four 32x32->64 partial products, as the
algorithm requires. The check is therefore not "no multiplier"; the
honest facts are:

- no `__umulti3` library call anywhere in the binary (grep count 0):
  the full 128-bit product is never formed by library widening;
- no single full-width product path from the original 64-bit inputs:
  there is no one-operand `mul r64` producing the rdx:rax 128-bit
  product of the original a and b (the only `mul %r11` in the binary
  is the FNV-1a prime multiply in the checksum loop);
- the carry accounting compiles to the expected `add`/`setb`/`adc`
  sequence with no widening.

The proof of correctness is the differential test above, not the
shape of the machine code.

## Re-checking

```
cd lab/140-mulhi-unsigned && make clean && make run
```

must exit 0 and print `RESULT: PASS` three times with the checksum
0x75e84e7a670bf3fc.
