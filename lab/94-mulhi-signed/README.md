# lab/94-mulhi-signed

`mulhi_s64(a, b)` in `mulhi_signed.h`: the high 64 bits of the signed
128-bit product of two `int64_t` values, built from the 32-bit
splitting identity plus exact sign corrections.

Construction. Write ua = (uint64_t)a, ub = (uint64_t)b, the same bit
patterns read unsigned, and sa = (a < 0), sb = (b < 0).  As integers,
a = ua - sa*2^64 and b = ub - sb*2^64, so

    a*b = ua*ub - sa*2^64*ub - sb*2^64*ua + sa*sb*2^128.

Every term is a multiple of 2^64 except the low word of ua*ub, hence

    high(a*b) = mulhi_u64(ua,ub) - sa*ub - sb*ua   (mod 2^64),

where the sa*sb*2^64 term vanishes mod 2^64.  The true high word
satisfies |high| <= 2^62 (since |a*b| <= 2^126), so the wrapped
64-bit result read back as int64_t is exact.  The unsigned high word
mulhi_u64 is the 32-bit splitting identity from lab/70: four
32-bit x 32-bit partial products (each below 2^64) combined with
explicit wrap detection, `(x + y) < x` is 1 exactly when the addition
wrapped.

This is what differs from the unsigned case: a negative operand
carries an implicit -2^64 from the sign extension of its high half,
and that term's product with the other operand lands entirely in the
high word.  The corrections are two conditional subtracts; the only
multiplies are the four unsigned 32x32 partial products.  No 128-bit
type and no full-width signed multiply appear in the implementation;
`signed __int128` is used only in the test's exact oracle.

Verified by `test_mulhi_signed.c`, differential against the signed
`__int128` reference:

- Exhaustive: all 4,294,967,296 pairs of 16-bit inputs,
  sign-extended to int64 (0 mismatches).  Every sign combination is
  covered, so each correction path (neither / a only / b only / both
  negative) runs over the full 16-bit range, and the high-half sign
  extension (a negative 16-bit value has high half 0xFFFFFFFF) is
  exercised in every case.
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0x123456789ABCDEF0`) 64-bit pairs, 0 mismatches.
- Directed: sign-boundary rows (INT64_MIN, INT64_MIN+1, +-2^33,
  +-2^32 +- 1, +-65536 +- 1, +-2, +-1, 0, INT64_MAX-1, INT64_MAX)
  as a 23x23 cross product, including INT64_MIN x INT64_MIN,
  INT64_MIN x -1, INT64_MIN x INT64_MAX, (-1) x (-1); plus signed
  powers of two (+-2^k, k = 0..62) in all three sign combinations
  (23x23 = 529 boundary pairs + 3x63x63 = 11,907 signed power pairs
  = 12,436 directed cases).
- 4,304,979,732 checks per build, 0 mismatches, in each of the `-O0`,
  `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every result word is identical across all
  three builds: `0x3282d931333f6a85`.
- Disassembly of the implementation at `-O2` (x86-64, GCC): four
  two-operand `imul` on the zero-extended 32-bit halves (exact
  32x32 -> 64-bit partial products), carries folded through
  `add`/`setb`/`adc`, and the sign corrections as branchless
  `test`/`cmovs` conditional subtracts.  No one-operand `mul`
  (128-bit rdx:rax result) and no 64x64 multiply of the original
  operands appears: the full-width signed multiply never re-forms.
- Throughput at `-O2`: best of 5 passes of 1M pairs, 5.942 ns/pair
  (passes: 8.249, 6.646, 5.942, 6.293, 6.792).  Pairs pre-generated
  with splitmix64; results xored into a volatile sink.
  `-O0`: 41.724 ns/pair; ASan+UBSan: 7.696 ns/pair.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full case set.
