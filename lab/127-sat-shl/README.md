# lab/127-sat-shl

`satshl.c` (`satshl.h` declares it): saturating 64-bit left shift,
`uint64_t sat_shl64(uint64_t x, unsigned k)`. Shifting left loses bits
off the top; when any bit is lost the result saturates to
`UINT64_MAX`, otherwise it is `x << k`. For `k >= 64` the result is
`UINT64_MAX` when `x != 0` and 0 when `x == 0`.

How it works, with no data-dependent branches (no `if`/`else`, no
ternary, no short-circuit on the data path; every decision is a 0/1
integer combined with shifts, masks, and ORs):

- `kk = k & 63` keeps every shift amount in `0..63`, so no C shift is
  ever performed with an amount of 64 or more.
- The bits shifted out of the top for `k` in `1..63` are exactly
  `x >> (64 - k)`, computed as `(x >> (63 - kk)) >> 1` so the shift
  amounts stay in range and the result is 0 when `kk == 0` (the first
  shift yields 0 or 1 there, the second clears it).
- Overflow is `(x != 0)` for `k >= 64` and `(lost != 0)` otherwise,
  selected with 0/1 masks, then `shifted | (ovf_mask & (MAX ^
  shifted))` saturates: where the mask is all ones the OR is
  `UINT64_MAX`, where it is zero the expression is `shifted`.

Verified by `test_satshl.c` (fixed-seed splitmix64 PRNG, seed
`0x123456789ABCDEF0`, fully reproducible):

- 5,194,304 differential checks, 0 mismatches: exhaustive sweep over
  all 64 shift amounts `k = 0..63` against all 65,536 16-bit `x`
  values (4,194,304 cases), plus 1,000,000 random 64-bit `x` values
  with `k` in `0..127`, against a plain conditional oracle
  (`if k >= 64 ...`, `if ((x >> (64 - k)) != 0) ...`, else `x << k`).
- Identical FNV-1a checksum (`10430319035230998775`) under `-O0`,
  `-O2`, and ASan+UBSan; zero sanitizer reports.
- Branchlessness: the `-O2` object for `sat_shl64` contains 0
  conditional jumps (programmatic `objdump` scan in the Makefile
  `disasm` target, which fails the build otherwise); the compiler
  emitted only `setne`/`cmove`/shift/neg/or.

Timing at `-O2` (best of 5 trials, 20,000,000 timed values per
trial, each drawn from the PRNG and accumulated into a sink so the
loop cannot be optimized away): 6.199 ns/value. Honest caveat: the
figure includes two PRNG steps per case, so it is the cost of one
generate-and-shift case, not one bare `sat_shl64` call.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then `make run` and `make disasm`. See `PROOF.md`
for the genuine build log and run output.
