# PROOF: lab/99-avg-round-half

`avg_rhu_u64(a, b)`: round-half-up average of two `uint64_t` values,
computed without ever forming `a + b`.

## What was built

`avg_round_half.h`, `test_avg_round_half.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`, no builtins.
The `unsigned __int128` exact oracle lives in the test program only;
the implementation uses nothing but 64-bit `&`, `^`, `>>`, `+`.

The implementation rests on one identity. For each bit position i, the
input bit pair (a_i, b_i) contributes a_i + b_i to the sum: 0, 1, or 2.
(a_i & b_i) is 1 exactly when the pair contributes 2 (a carry into
position i + 1), and (a_i ^ b_i) is 1 exactly when the pair
contributes 1. Summing over all positions gives the ordinary integer
equality for the exact (unbounded) sum s = a + b:

    s = 2 * (a & b) + (a ^ b).

With y = a & b and x = a ^ b, s = 2y + x. Rounding half up is
rhu(q) = floor(q + 1/2), so

    rhu(s / 2) = floor(y + x/2 + 1/2) = y + floor(x/2 + 1/2)

because y is an integer. For integer x, floor(x/2 + 1/2) is x >> 1
when x is even and (x >> 1) + 1 when x is odd; both cases are
(x >> 1) + (x & 1). Hence

    rhu((a + b) / 2) = (a & b) + ((a ^ b) >> 1) + ((a ^ b) & 1),

where the last term adds 1 exactly when x is odd, i.e. exactly when
the exact sum s is odd (the .5 case), which is the tie that rounding
half up pushes to the ceiling.

Overflow analysis, step by step:

1. y + (x >> 1) equals floor((a + b) / 2) exactly (the lab/59
   decomposition), so it is at most 2^64 - 1 and that addition
   cannot wrap.
2. The correction (x & 1) is 0 or 1. When it is 1, s is odd.
3. An odd sum satisfies s <= 2^65 - 3, because the maximum possible
   sum 2^65 - 2 is even.
4. Therefore when the correction is 1, the floor part is at most
   (2^65 - 3 - 1) / 2 = 2^64 - 2, and the final +1 stays below 2^64.

All operands are unsigned, so there is no signed overflow and no
implementation-defined shift anywhere.

## Verification plan

1. Phase 1: exhaustive over all 2^32 pairs of uint16_t operands,
   differential-checked against the exact __int128 oracle
   floor((s + 1) / 2).
2. Phase 2: 13 directed tie rows, odd-sum cases where the exact
   average ends in .5, printed as a table to pin the tie behavior
   (including (0,1), (UINT64_MAX,0), (UINT64_MAX,UINT64_MAX-1), and
   (UINT64_MAX,UINT64_MAX)); each row differential-checked.
3. Phase 3: 10,000,000 fixed-seed splitmix64 64-bit pairs (seed
   0x123456789ABCDEF0), each differential-checked.
4. Every result folded into a 64-bit FNV-1a checksum (byte at a time,
   low byte first); the checksum must be identical across -O0, -O2,
   and ASan+UBSan builds.

## Build log (verbatim)

```
$ make
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_avg_round_half test_avg_round_half.c
$ make test_avg_round_half_O0 test_avg_round_half_san test_avg_round_half_bench
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_avg_round_half_O0 test_avg_round_half.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_avg_round_half_san test_avg_round_half.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_avg_round_half_bench test_avg_round_half.c
```

Zero warnings under -Wall -Wextra -Werror on all four targets.

## Run output, -O2 (verbatim)

```
exhaustive: a=    0 of 65535, pairs=65536, mismatches=0
exhaustive: a=16384 of 65535, pairs=1073807360, mismatches=0
exhaustive: a=32768 of 65535, pairs=2147549184, mismatches=0
exhaustive: a=49152 of 65535, pairs=3221291008, mismatches=0
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 136.7 s (31.82 ns/pair)
phase 2 (tie rows, odd-sum cases round up):
                   a                    b  tie                  got                 want ok
                   0                    0    0                    0                    0 ok
                   0                    1    1                    1                    1 ok
                   1                    0    1                    1                    1 ok
                   1                    1    0                    1                    1 ok
                   2                    3    1                    3                    3 ok
18446744073709551615                    0    1  9223372036854775808  9223372036854775808 ok
18446744073709551615                    1    0  9223372036854775808  9223372036854775808 ok
18446744073709551615 18446744073709551614    1 18446744073709551615 18446744073709551615 ok
18446744073709551615 18446744073709551615    0 18446744073709551615 18446744073709551615 ok
 9223372036854775808  9223372036854775809    1  9223372036854775809  9223372036854775809 ok
 6148914691236517205 12297829382473034410    1  9223372036854775808  9223372036854775808 ok
12297829382473034410 12297829382473034410    0 12297829382473034410 12297829382473034410 ok
                   1 18446744073709551615    0  9223372036854775808  9223372036854775808 ok
phase 2: 13 tie rows, 0 mismatches
phase 3 (10M random 64-bit pairs): 10000000 pairs, 0 mismatches, 0.2 s (24.44 ns/pair)
correctness: 4304967309 differential checks, 0 mismatches, fnv1a=1f09d1d42620ee61
ALL TESTS PASSED
```

## Run output, -O0 and ASan+UBSan (final lines, verbatim)

-O0:

```
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 223.8 s (52.11 ns/pair)
phase 3 (10M random 64-bit pairs): 10000000 pairs, 0 mismatches, 0.4 s (36.72 ns/pair)
correctness: 4304967309 differential checks, 0 mismatches, fnv1a=1f09d1d42620ee61
ALL TESTS PASSED
```

ASan+UBSan (`-fsanitize=address,undefined -fno-sanitize-recover=all`):

```
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 194.1 s (45.19 ns/pair)
phase 3 (10M random 64-bit pairs): 10000000 pairs, 0 mismatches, 0.5 s (50.65 ns/pair)
correctness: 4304967309 differential checks, 0 mismatches, fnv1a=1f09d1d42620ee61
ALL TESTS PASSED
```

No sanitizer reports (`grep -iE 'runtime error|AddressSanitizer|
UndefinedBehaviorSanitizer'` over the sanitizer log: no matches).

## -O2 machine code for the function (verbatim, objdump)

Compiled as a standalone TU so the inlined body is visible; the
compiler emits exactly the and/xor/shift/add sequence, no division and
no widening:

```
0000000000000000 <f>:
   0:	endbr64
   4:	mov    %rdi,%rdx
   7:	and    %rsi,%rdi
   a:	xor    %rsi,%rdx
   d:	mov    %rdx,%rax
  10:	and    $0x1,%edx
  13:	shr    $1,%rax
  16:	add    %rdx,%rax
  19:	add    %rdi,%rax
  1c:	ret
```

(`grep -c int128 avg_round_half.h`: 0; the oracle's `__int128` appears
only in `test_avg_round_half.c`.)

## Throughput

```
$ make bench && ./test_avg_round_half_bench
bench: 10.11 ns/value (98.9 Mvalues/s over 25M timed values, best of 5)
```

Each timed value draws two fresh 64-bit splitmix64 operands, so this
measures the avg plus two RNG steps.

## Exactly what was verified

- 4,304,967,309 differential checks (2^32 exhaustive 16-bit pairs +
  13 tie rows + 10,000,000 random 64-bit pairs), 0 mismatches against
  the exact __int128 round-half-up oracle.
- Checksum `1f09d1d42620ee61` identical across -O0, -O2, and
  ASan+UBSan; no sanitizer reports.
- Tie rows pin the exact tie behavior: every odd-sum row has
  tie = 1 and the result equals the ceiling of the exact average
  (e.g. (0,1) -> 1, (UINT64_MAX,0) -> 2^63, (UINT64_MAX,UINT64_MAX-1)
  -> 2^64 - 1); even-sum rows have tie = 0.
- -O2 machine code is exactly the and/xor/shr/add sequence above.
- Throughput 10.11 ns/value at -O2 (best of 5).
- Build is warning-free under -Wall -Wextra -Werror.

Built and tested 2026-09-10. Toolchain: gcc 13.3.0 (Ubuntu) on
x86-64.
