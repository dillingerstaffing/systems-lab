<!-- PROOF-HEADER
Checks: 10065537
Mismatches: 0
Checksum: bb15711279c99def
Throughput: 14.29 ns/value at -O2 (best of 5)
Environment: Host
Verdict: PASS
-->
# PROOF: lab/115-clz-fp

`clz64_fp(x)`: count leading zeros of a `uint64_t`, computed by
filling `x` upward and reading the unbiased exponent of `(double)x`
from the IEEE-754 binary64 bit layout.

## What was built

`clz_fp.h`, `clz_fp.c`, `test_clz_fp.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`. No
intrinsics, no builtins, no library math in the implementation; the
`double` bits are read through a union. `__builtin_clzll` is used
only as the test oracle, never in `clz_fp.c`.

## Derivation

Definitions: `n = floor(log2(x))` for `x > 0`, so `clz(x) = 63 - n`.
Binary64: 1 sign bit, 11 exponent bits with bias 1023, 52 stored
mantissa bits; value `1.f * 2^(E-1023)`.

Step 1, fill. `x |= x >> k` for k = 1, 2, 4, 8, 16, 32 copies the
top set bit downward; each step doubles the run of 1s below the top
bit. After all six, every bit at or below the top set bit is 1, so
the filled value is exactly `2^(n+1) - 1` (bits 0..n all set).

Step 2, read the exponent. Cast the filled value to `double`
(rounds to nearest, ties to even, under the default rounding mode)
and take `(bits >> 52) & 0x7FF`, minus 1023.

Step 3, the rounding correction. This is the hazard the whole lab
turns on, derived here rather than assumed:

- `n <= 52`: filled value `2^(n+1) - 1 <= 2^53 - 1 < 2^53`, converts
  exactly. Exponent field reads `n`. No correction.
- `n = 53`: filled value `2^54 - 1`. Doubles on `[2^53, 2^54)` are
  spaced 2 apart, so this is an exact midpoint between `2^54 - 2`
  and `2^54`. The significand of `2^54 - 2` is `1.111...1` with 52
  ones after the point (since `1.11...1` with k ones equals
  `2 - 2^-k`, and `(2^54 - 2)/2^53 = 2 - 2^-52`), so its least
  significant bit is 1 (odd). The significand of `2^54` is `1.0`,
  least bit 0 (even). Round-ties-to-even picks the even one:
  `2^54`. Exponent field reads 54 = n + 1. One too high.
- `n >= 54`: doubles on `[2^n, 2^(n+1))` are spaced `2^(n-52)`.
  The two neighbors of the filled value `2^(n+1) - 1` are
  `2^(n+1) - 2^(n-52)` below (distance `2^(n-52) - 1`, which is at
  least 3 for `n >= 54`) and `2^(n+1)` above (distance 1). The cast
  rounds up to `2^(n+1)`. Exponent field reads `n + 1`. One too
  high.

In every case with `n >= 53` the field reads exactly one too high
(the rounded value is at most `2^(n+1)`, so it can never read two
too high), and filling only sets bits at or below the top set bit,
so `n >= 53` on the filled value holds iff the original `x >= 2^53`,
tested as `(x >> 53) != 0`. The correction is `if (x >> 53) n -= 1;`
then `return 63 - n`.

Step 4, zero. `x = 0` is pinned to 64 by contract and handled before
any conversion (`(double)0` is `+0.0` with exponent field 0, which
would underflow the bias subtraction).

## The test caught the hazard, then confirmed the fix

The first draft of the correction used `x >> 54`, on the mistaken
belief that the `n = 53` midpoint would tie down to `2^54 - 2`. The
differential test caught it: 5010 mismatches out of 10,000,000,
every one at `n = 53` (for example `v = 15292517325070885` read 9,
oracle said 10). Re-deriving the tie (significand parity above)
showed ties-to-even picks `2^54`, the correction was widened to
`x >> 53`, and the rerun below is clean. The test is sensitive to
exactly the hazard it exists to check.

An independent probe printed the raw exponent field read for
boundary inputs, confirming the derivation on this host:

```
x=                   1 n= 0 filled=                   1 dbl_exp_read=  0
x=                   2 n= 1 filled=                   3 dbl_exp_read=  1
x=    4503599627370496 n=52 filled=    9007199254740991 dbl_exp_read= 52
x=    9007199254740991 n=52 filled=    9007199254740991 dbl_exp_read= 52
x=    9007199254740992 n=53 filled=   18014398509481983 dbl_exp_read= 54
x=    9007199254740993 n=53 filled=   18014398509481983 dbl_exp_read= 54
x=   18014398509481983 n=53 filled=   18014398509481983 dbl_exp_read= 54
x=   18014398509481984 n=54 filled=   36028797018963967 dbl_exp_read= 55
x= 9223372036854775808 n=63 filled=18446744073709551615 dbl_exp_read= 64
x=18446744073709551615 n=63 filled=18446744073709551615 dbl_exp_read= 64
```

Reads `n` for `n <= 52`, `n + 1` for `n >= 53`: exactly the two
cases in the derivation.

## Build log (genuine output)

```
$ make test_clz_fp test_clz_fp_O0 test_clz_fp_san test_clz_fp_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_clz_fp test_clz_fp.c clz_fp.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_clz_fp_O0 test_clz_fp.c clz_fp.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_clz_fp_san test_clz_fp.c clz_fp.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_clz_fp_bench test_clz_fp.c clz_fp.c
```

Zero warnings on all four builds (warnings are errors).

## Run logs (genuine output)

`-O2`:
```
x=0 contract: clz64_fp(0) = 64 OK
checks: 10065537
mismatches: 0
checksum: bb15711279c99def
```

`-O0`:
```
x=0 contract: clz64_fp(0) = 64 OK
checks: 10065537
mismatches: 0
checksum: bb15711279c99def
```

ASan+UBSan (`-O2 -fsanitize=address,undefined -fno-sanitize-recover=all`):
```
x=0 contract: clz64_fp(0) = 64 OK
checks: 10065537
mismatches: 0
checksum: bb15711279c99def
```

No sanitizer reports on stderr; exit status 0 on all three. The
checksum is byte-identical across all three builds.

Throughput (`test_clz_fp_bench`, 1M-value buffer, 5 reps, -O2):
```
rep 0: 15.234 ns/value (sink 0)
rep 1: 14.292 ns/value (sink 783842)
rep 2: 14.454 ns/value (sink 0)
rep 3: 20.319 ns/value (sink 783842)
rep 4: 17.473 ns/value (sink 0)
best: 14.292 ns/value
```

## What was verified

- All 65,536 16-bit inputs, exhaustive, differential against
  `__builtin_clzll`.
- `x = 0` separately against the 64 contract.
- 10,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential against
  `__builtin_clzll`. The 64-bit stream densely covers `n >= 53`
  (roughly half the values), so the rounding correction is
  exercised on the order of five million inputs.
- 10,065,537 total checks, 0 mismatches. The derivation above
  assumes the default round-to-nearest-even mode; the probe table
  and the clean 10M run confirm this host converts that way.
