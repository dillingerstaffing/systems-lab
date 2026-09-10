# lab/125: unsigned division by 7 from the shift-add series

`udiv7_shiftadd(uint32_t x)` in `div7.h` computes `x / 7` using only shifts
and adds. No division or multiplication operators appear in the
implementation, and the `-O2` codegen contains no `div`/`idiv` instruction
(verified by `make disasm`, which fails the build otherwise).

## Idea

In binary, the reciprocal of 7 is the repeating fraction `0.001001001...`,
because the geometric series 2^-3 + 2^-6 + 2^-9 + ... sums to exactly 1/7.
Multiplying through by x:

    x / 7 = (x >> 3) + (x >> 6) + (x >> 9) + ...   (as exact reals)

The implementation sums the integer parts of the first ten terms
(`x >> 3` through `x >> 30`; later terms shift a 32-bit value to zero).
That estimate never exceeds x/7 and falls short by less than 10.58, so it
starts at most 10 below the true quotient. A short correction loop then
walks it up to the exact value, comparing `7q` (built as
`q + (q << 1) + (q << 2)`) against x. Full derivation in PROOF.md.

## Verification

Differential test against the C `/` operator:

- 22 targeted edge cases (0, 1, 6, 7, 8, values straddling 2^24, values
  straddling UINT32_MAX)
- exhaustive: all 2^24 inputs, 0 to 16,777,215
- 10,000,000 fixed-seed splitmix64 32-bit values (seed 0x123456789ABCDEF0)

Total 26,777,238 checks, 0 mismatches, across three builds (`-O0`, `-O2`,
ASan+UBSan), all producing the identical FNV-1a result checksum
`0x835b51585dc7f434`. Maximum correction steps observed: 10, matching the
proven bound. Throughput at `-O2`: 16.600 ns/value (best of 5, 50M values).

## Files

- `div7.h`       the implementation (shifts and adds only)
- `test_div7.c`  differential test vs `/`, edge cases, checksum
- `bench_div7.c` throughput benchmark
- `impl_check.c` minimal TU used by the no-div disassembly check
- `Makefile`     builds everything; `make check` runs all three test builds
- `PROOF.md`     derivation, genuine build/test logs, edge-case contract
