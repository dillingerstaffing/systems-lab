# lab/122-decimal-digits

Decimal digit count of a 64-bit word, as a straight-line comparison
cascade: `digit_count(x) = 1 + (x >= 10) + (x >= 100) + ... + (x >= 10^19)`.
Each comparison is a magnitude truth about `x`: it is 1 exactly when `x`
reaches that power of ten, so the sum counts how many powers of ten `x`
reaches and adding 1 gives the digit count. The implementation uses no
loop, no division, no table, and no string formatting; gcc 13.3.0 at
`-O2` compiles it to a `cmp`/`adc` sequence with 0 jump and 0 division
instructions (see the programmatic scans in `make disasm`). Plain C11,
`-std=c11 -Wall -Wextra -Werror`, no intrinsics, no builtins.

Verified in `test_decimal_digits.c` against the `strlen` of
`snprintf(buf, "%llu")` as oracle (a structurally different computation:
formatting code in libc, not a comparison cascade):

- 40 directed boundary rows: `x = 0`, `10^k - 1` and `10^k` for
  `k = 1..19`, and `UINT64_MAX` (which has 20 digits).
- 10,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`).
- Every 32-bit value `0 .. 2^32 - 1`: the full sweep, 4,294,967,296
  checks.
- 0 mismatches across all 4,304,967,336 checks.
- FNV-1a checksum `2bb5269a79ea9099` over every implementation output
  byte, byte-identical across `-O0`, `-O2`, and ASan+UBSan; zero
  sanitizer reports; build is warning-free under
  `-Wall -Wextra -Werror`.
- Throughput 11.55 ns/value at `-O2` (best of 5 over 25M values).
