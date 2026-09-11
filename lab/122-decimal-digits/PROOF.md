<!-- PROOF-HEADER
Checks: 4304967336
Mismatches: 0
Checksum: 2bb5269a79ea9099
Throughput: 11.55 ns/value at -O2, best of 5
Environment: Host
Verdict: PASS
-->
# PROOF: lab/122-decimal-digits

`digit_count(x) = 1 + (x >= 10) + (x >= 100) + ... + (x >= 10^19)` for
64-bit `x`: the decimal digit count of `x`, computed as a straight-line
comparison cascade with no loop, no division, no table, and no string
formatting in the implementation.

## What was built

`decimal_digits.h`, `decimal_digits.c`, `test_decimal_digits.c`,
`Makefile`, `README.md`, this file. Plain C11,
`-std=c11 -Wall -Wextra -Werror`, no intrinsics, no builtins, no
library calls in the implementation. The oracle in the test is
`strlen` of `snprintf(buf, "%llu")`, a structurally different
computation (libc's formatting code versus a comparison cascade), so
agreement pins the implementation rather than a shared bug.

## The construction

From `decimal_digits.c`:

```c
return (uint8_t)(1
    + (x >= 10ULL)
    + (x >= 100ULL)
    + (x >= 1000ULL)
    /* ... one comparison per power of ten ... */
    + (x >= 1000000000000000000ULL)
    + (x >= 10000000000000000000ULL));
```

The reasoning: `10^k - 1` is the largest k-digit number and `10^k` is
the smallest (k+1)-digit number, so `(x >= 10^k)` is 1 exactly when `x`
has more than k digits. Summing over k = 1..19 counts how many powers
of ten `x` reaches; adding 1 gives the digit count. `10^19` is the
largest power of ten that fits in a `uint64_t`
(10000000000000000000 < 2^64), so 19 comparisons cover every input,
and the maximum result is 20, which fits in the `uint8_t` return.
Each `(x >= 10^k)` is an unsigned comparison, defined for every
input; the C expression `(x >= 10^k)` evaluates to exactly 0 or 1.

The `-O2` object code of `digit_count` (gcc 13.3.0, x86-64) is a
`cmp`/`adc` sequence: each comparison sets the carry flag and `adc`
accumulates it, which is the comparison cascade with no branch.
The programmatic scans (`make disasm`) reported
`jump instructions found: 0` and `division instructions found: 0`
over the function's disassembly.

## Exactly what was verified

- 4,304,967,336 differential checks, 0 mismatches against the
  `snprintf` oracle:
  - 40 directed boundary rows: `x = 0`, `10^k - 1` and `10^k` for
    `k = 1..19`, and `UINT64_MAX` (20 digits),
  - 10,000,000 fixed-seed `splitmix64` 64-bit values (seed
    `0x123456789ABCDEF0`),
  - 4,294,967,296: every 32-bit value `0 .. 2^32 - 1`, the full sweep.
- FNV-1a checksum `2bb5269a79ea9099` over every implementation
  output byte, identical across `-O0`, `-O2`, and ASan+UBSan; no
  sanitizer reports.
- Throughput 11.55 ns/value at `-O2` (best of 5 over 25M values).
- Build is warning-free under `-Wall -Wextra -Werror`; the `make
  disasm` scans confirm 0 jump and 0 division instructions in
  `digit_count` at `-O2`.

## Build and run logs (actual output)

```
=== build log ===
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_decimal_digits test_decimal_digits.c decimal_digits.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_decimal_digits_O0 test_decimal_digits.c decimal_digits.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_decimal_digits_san test_decimal_digits.c decimal_digits.c
=== run: -O2 ===
phase1 boundaries (x, expected, got):
  x=                   0 expected= 1 got= 1
  x=                   9 expected= 1 got= 1
  x=                  10 expected= 2 got= 2
  x=                  99 expected= 2 got= 2
  x=                 100 expected= 3 got= 3
  x=                 999 expected= 3 got= 3
  x=                1000 expected= 4 got= 4
  x=                9999 expected= 4 got= 4
  x=               10000 expected= 5 got= 5
  x=               99999 expected= 5 got= 5
  x=              100000 expected= 6 got= 6
  x=              999999 expected= 6 got= 6
  x=             1000000 expected= 7 got= 7
  x=             9999999 expected= 7 got= 7
  x=            10000000 expected= 8 got= 8
  x=            99999999 expected= 8 got= 8
  x=           100000000 expected= 9 got= 9
  x=           999999999 expected= 9 got= 9
  x=          1000000000 expected=10 got=10
  x=          9999999999 expected=10 got=10
  x=         10000000000 expected=11 got=11
  x=         99999999999 expected=11 got=11
  x=        100000000000 expected=12 got=12
  x=        999999999999 expected=12 got=12
  x=       1000000000000 expected=13 got=13
  x=       9999999999999 expected=13 got=13
  x=      10000000000000 expected=14 got=14
  x=      99999999999999 expected=14 got=14
  x=     100000000000000 expected=15 got=15
  x=     999999999999999 expected=15 got=15
  x=    1000000000000000 expected=16 got=16
  x=    9999999999999999 expected=16 got=16
  x=   10000000000000000 expected=17 got=17
  x=   99999999999999999 expected=17 got=17
  x=  100000000000000000 expected=18 got=18
  x=  999999999999999999 expected=18 got=18
  x= 1000000000000000000 expected=19 got=19
  x= 9999999999999999999 expected=19 got=19
  x=10000000000000000000 expected=20 got=20
  x=18446744073709551615 expected=20 got=20
phase2 random: done
phase3 full 2^32 sweep:   phase3 progress: 0 / 4294967296
  phase3 progress: 1073741824 / 4294967296
  phase3 progress: 2147483648 / 4294967296
  phase3 progress: 3221225472 / 4294967296
done
total checks: 4304967336
mismatches: 0
checksum: 2bb5269a79ea9099
PASS
=== run: -O0 ===
phase1 boundaries (x, expected, got):
  x=                   0 expected= 1 got= 1
  x=                   9 expected= 1 got= 1
  x=                  10 expected= 2 got= 2
  x=                  99 expected= 2 got= 2
  x=                 100 expected= 3 got= 3
  x=                 999 expected= 3 got= 3
  x=                1000 expected= 4 got= 4
  x=                9999 expected= 4 got= 4
  x=               10000 expected= 5 got= 5
  x=               99999 expected= 5 got= 5
  x=              100000 expected= 6 got= 6
  x=              999999 expected= 6 got= 6
  x=             1000000 expected= 7 got= 7
  x=             9999999 expected= 7 got= 7
  x=            10000000 expected= 8 got= 8
  x=            99999999 expected= 8 got= 8
  x=           100000000 expected= 9 got= 9
  x=           999999999 expected= 9 got= 9
  x=          1000000000 expected=10 got=10
  x=          9999999999 expected=10 got=10
  x=         10000000000 expected=11 got=11
  x=         99999999999 expected=11 got=11
  x=        100000000000 expected=12 got=12
  x=        999999999999 expected=12 got=12
  x=       1000000000000 expected=13 got=13
  x=       9999999999999 expected=13 got=13
  x=      10000000000000 expected=14 got=14
  x=      99999999999999 expected=14 got=14
  x=     100000000000000 expected=15 got=15
  x=     999999999999999 expected=15 got=15
  x=    1000000000000000 expected=16 got=16
  x=    9999999999999999 expected=16 got=16
  x=   10000000000000000 expected=17 got=17
  x=   99999999999999999 expected=17 got=17
  x=  100000000000000000 expected=18 got=18
  x=  999999999999999999 expected=18 got=18
  x= 1000000000000000000 expected=19 got=19
  x= 9999999999999999999 expected=19 got=19
  x=10000000000000000000 expected=20 got=20
  x=18446744073709551615 expected=20 got=20
phase2 random: done
phase3 full 2^32 sweep:   phase3 progress: 0 / 4294967296
  phase3 progress: 1073741824 / 4294967296
  phase3 progress: 2147483648 / 4294967296
  phase3 progress: 3221225472 / 4294967296
done
total checks: 4304967336
mismatches: 0
checksum: 2bb5269a79ea9099
PASS
=== run: ASan+UBSan ===
phase1 boundaries (x, expected, got):
  x=                   0 expected= 1 got= 1
  x=                   9 expected= 1 got= 1
  x=                  10 expected= 2 got= 2
  x=                  99 expected= 2 got= 2
  x=                 100 expected= 3 got= 3
  x=                 999 expected= 3 got= 3
  x=                1000 expected= 4 got= 4
  x=                9999 expected= 4 got= 4
  x=               10000 expected= 5 got= 5
  x=               99999 expected= 5 got= 5
  x=              100000 expected= 6 got= 6
  x=              999999 expected= 6 got= 6
  x=             1000000 expected= 7 got= 7
  x=             9999999 expected= 7 got= 7
  x=            10000000 expected= 8 got= 8
  x=            99999999 expected= 8 got= 8
  x=           100000000 expected= 9 got= 9
  x=           999999999 expected= 9 got= 9
  x=          1000000000 expected=10 got=10
  x=          9999999999 expected=10 got=10
  x=         10000000000 expected=11 got=11
  x=         99999999999 expected=11 got=11
  x=        100000000000 expected=12 got=12
  x=        999999999999 expected=12 got=12
  x=       1000000000000 expected=13 got=13
  x=       9999999999999 expected=13 got=13
  x=      10000000000000 expected=14 got=14
  x=      99999999999999 expected=14 got=14
  x=     100000000000000 expected=15 got=15
  x=     999999999999999 expected=15 got=15
  x=    1000000000000000 expected=16 got=16
  x=    9999999999999999 expected=16 got=16
  x=   10000000000000000 expected=17 got=17
  x=   99999999999999999 expected=17 got=17
  x=  100000000000000000 expected=18 got=18
  x=  999999999999999999 expected=18 got=18
  x= 1000000000000000000 expected=19 got=19
  x= 9999999999999999999 expected=19 got=19
  x=10000000000000000000 expected=20 got=20
  x=18446744073709551615 expected=20 got=20
phase2 random: done
phase3 full 2^32 sweep:   phase3 progress: 0 / 4294967296
  phase3 progress: 1073741824 / 4294967296
  phase3 progress: 2147483648 / 4294967296
  phase3 progress: 3221225472 / 4294967296
done
total checks: 4304967336
mismatches: 0
checksum: 2bb5269a79ea9099
PASS
=== ALL DONE rc=0 ===
```
