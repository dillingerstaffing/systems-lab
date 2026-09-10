# lab/86-saturating-add

`sat_add32.h`: 32-bit saturating addition, `int32_t` in, clamped to
`INT32_MIN`/`INT32_MAX` on signed overflow instead of wrapping.

The sum is formed in unsigned arithmetic, where wraparound is defined
and can never be undefined behavior. Overflow is detected from the
sign bits alone, via the two's complement identity: `(a + b)`
overflows exactly when both addends share a sign and the wrapped sum
carries the opposite sign. Formally, bit 31 of
`((ua ^ sum) & (ub ^ sum))` is set exactly on overflow, where `ua`,
`ub` are the addends as `uint32_t` and `sum` is their wrapped sum. With
mixed signs the true sum lies strictly between the two operands, so
overflow is impossible there. On overflow both addends share one sign,
so the clamp value is selected from the sign of `a` (`INT32_MIN` for
negative, `INT32_MAX` for positive), with a branchless mask so every
input takes the identical instruction path (see the disassembly in
PROOF.md: no jumps, `lea`/`xor`/`and`/`sar`/`or` only).

Verified by `test_satadd.c` (deterministic splitmix64 PRNG with a
fixed seed, fully reproducible), differential against the oracle: the
exact sum in `int64_t` (which cannot overflow for 32-bit operands),
clamped to `[INT32_MIN, INT32_MAX]`:

- 1,065,547 total checks, 0 mismatches, identical FNV-1a checksums
  under `-O0`, `-O2`, and ASan+UBSan (see PROOF.md).
- 11 known-answer checks covering every clamp corner (`INT32_MAX + 1`,
  `INT32_MIN - 1`, `INT32_MAX + INT32_MAX`, `0 - INT32_MIN` via
  `INT32_MIN + 0`, and friends).
- 1,000,000 fixed-seed random full-range pairs.
- 65,536 directed edge pairs: the cross product of 256 boundary
  values (32 at each of `INT32_MIN + i`, `INT32_MAX - i`,
  `-32768 + i`, `32767 - i`, plus the 128 values `-64`..`63`).

Timing (best of 5 runs over 200M timed pairs; the timed loop includes
PRNG generation and result hashing, as in past labs): `-O2`
5.146 ns/pair, `-O0` 12.408 ns/pair, ASan+UBSan 6.165 ns/pair.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-std=c11 -Wall -Wextra
-Werror`, zero warnings), `make run`. See `PROOF.md` for the genuine
build log and run output.

Note: lab/19-saturating-arith covers the same operation with a
different construction (branching select, shared with its saturating
sub). This lab is the branchless overflow-flag-identity variant as a
standalone module.
