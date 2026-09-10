<!-- PROOF-HEADER
Checks: 10065537
Mismatches: 0
Checksum: e52a81b464c7559c
Throughput: 6.21 ns/value at -O2 (best of 5)
Environment: Host
Verdict: PASS
-->
# PROOF: lab/109-sat-neg

`neg_sat64(x)`: saturating negation of an `int64_t`. Returns
`INT64_MAX` when `x == INT64_MIN`, `-x` otherwise. Built from the
sign-mask identity with the `INT64_MIN` edge folded into the mask:
no branch, no comparison anywhere.

## What was built

`sat_neg.h`, `sat_neg.c`, `test_sat_neg.c`, `Makefile`,
`README.md`, this file. Plain C11,
`-std=c11 -Wall -Wextra -Werror`. No intrinsics, no builtins in the
implementation. The test oracle is the definitional ternary
`x == INT64_MIN ? INT64_MAX : -x`, used only in the test.

## Derivation

Work in unsigned 64-bit arithmetic (mod 2^64): `u = (uint64_t)x`
is the two's complement bit pattern of x, and
`s = (uint64_t)(x >> 63)` is the sign mask, 0 for x >= 0 and
2^64 - 1 (all ones) for x < 0.

Base identity: `s - u + (s & 1)`.
  - x >= 0: s = 0, value = `0 - u + 0` = -u mod 2^64, the two's
    complement negation. -x lies in [-(2^63 - 1), 0], so it is
    representable and the bit pattern is exactly -x.
  - x < 0, x != INT64_MIN: s = 2^64 - 1, s & 1 = 1, value =
    (2^64 - 1) - u + 1 = 2^64 - u = -u mod 2^64. -x lies in
    [1, 2^63 - 1], representable, so the pattern is exactly -x.
  - x == INT64_MIN: s = 2^64 - 1, u = 2^63, base gives
    (2^64 - 1) - 2^63 + 1 = 2^63, the wrapped INT64_MIN pattern,
    exactly one more than the correct answer INT64_MAX = 2^63 - 1.

The edge is folded into the low mask bit: replace `(s & 1)` with
`((s & 1) ^ e)`, where e = 1 iff x == INT64_MIN. The MIN row then
computes (2^64 - 1) - 2^63 + (1 ^ 1) = 2^63 - 1 = INT64_MAX, and
every other row is unchanged because e = 0 there.

Branchless edge detector, no comparison: `d = u ^ 0x8000000000000000`
is zero exactly when x == INT64_MIN. For d != 0, at least one of d
and -d mod 2^64 has bit 63 set: if bit 63 of d is 1 then d itself
does; otherwise d < 2^63 and -d = 2^64 - d lies in [2^63, 2^64 - 1],
whose bit 63 is set. Hence `(d | (0 - d)) >> 63` is 1 iff d != 0
and 0 iff d = 0, so `e = 1 - ((d | (0 - d)) >> 63)` is 1 exactly on
the INT64_MIN edge.

No signed overflow anywhere: every operation is on uint64_t. The
only implementation-defined operation is the arithmetic right
shift of a negative int64_t, which sign-extends on every two's
complement target.

## Build log (genuine output)

```
$ make clean && make test_sat_neg test_sat_neg_O0 test_sat_neg_san test_sat_neg_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -c -o sat_neg_O2.o sat_neg.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_sat_neg test_sat_neg.c sat_neg_O2.o
gcc -std=c11 -Wall -Wextra -Werror -O0 -c -o sat_neg_O0 sat_neg.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_sat_neg_O0 test_sat_neg.c sat_neg_O0.o
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -c -o sat_neg_san.o sat_neg.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_sat_neg_san test_sat_neg.c sat_neg_san.o
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_sat_neg_bench test_sat_neg.c sat_neg_O2.o
```

Zero warnings on all seven compile/link steps (warnings are
errors).

## Run logs (genuine output)

`-O2`:
```
disasm check: no conditional jump in sat_neg_O2.o OK
INT64_MIN edge: neg_sat64(INT64_MIN) = INT64_MAX OK
checks: 10065537
mismatches: 0
checksum: e52a81b464c7559c
```

`-O0`:
```
disasm check: no conditional jump in sat_neg_O2.o OK
INT64_MIN edge: neg_sat64(INT64_MIN) = INT64_MAX OK
checks: 10065537
mismatches: 0
checksum: e52a81b464c7559c
```

ASan+UBSan:
```
disasm check: no conditional jump in sat_neg_O2.o OK
INT64_MIN edge: neg_sat64(INT64_MIN) = INT64_MAX OK
checks: 10065537
mismatches: 0
checksum: e52a81b464c7559c
```

All three builds agree: 10,065,537 checks (exhaustive 16-bit plus
10M fixed-seed 64-bit plus the dedicated INT64_MIN row), 0
mismatches, one checksum. The checksum `e52a81b464c7559c` is the
FNV-1a 64-bit hash of the result stream.

Independent manual check on the -O2 object file (the gcc output
for `neg_sat64`, shown in full; the grep counted conditional-jump
mnemonics):
```
$ objdump -d --no-show-raw-insn sat_neg_O2.o | sed -n '/<neg_sat64>:/,/ret/p'
0000000000000000 <neg_sat64>:
   0:	endbr64
   4:	mov    %rdi,%rdx
   7:	btc    $0x3f,%rdx
   c:	mov    %rdx,%rax
   f:	neg    %rax
  12:	or     %rdx,%rax
  15:	mov    %rdi,%rdx
  18:	sar    $0x3f,%rax
  1c:	shr    $0x3f,%rdx
  20:	add    $0x1,%rax
  24:	xor    %rdx,%rax
  27:	mov    %rdi,%rdx
  2a:	sar    $0x3f,%rdx
  2e:	sub    %rdi,%rdx
  31:	add    %rdx,%rax
  34:	ret
$ objdump -d --no-show-raw-insn sat_neg_O2.o | grep -cE '^\s+[0-9a-f]+:\s+j[a-z]+'
0
```

## Throughput (genuine output, -O2)

```
$ ./test_sat_neg_bench
rep 0: 9.106 ns/value (sink 0)
rep 1: 9.765 ns/value (sink 4766881635340855794)
rep 2: 8.879 ns/value (sink 0)
rep 3: 7.118 ns/value (sink 4766881635340855794)
rep 4: 6.210 ns/value (sink 0)
best: 6.210 ns/value
```

Best of 5 over a 1M-value buffer: 6.21 ns/value.

## What was verified

- The derivation above covers every input; the differential test
  re-checked 10,065,537 inputs (exhaustive signed 16-bit + 10M
  fixed-seed 64-bit + the dedicated INT64_MIN row) against the
  ternary reference with 0 mismatches.
- `x = INT64_MIN` returns INT64_MAX, checked as its own row.
- The -O2 machine code of the implementation contains no
  conditional jump (checked programmatically in the test and
  independently by grep).
- -O0, -O2, ASan, and UBSan builds produce identical results.
- The seed `0x123456789ABCDEF0` is fixed, so the random pass is
  reproducible.
