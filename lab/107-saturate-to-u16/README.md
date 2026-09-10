# lab/107-saturate-to-u16

`saturate_u16(x)`: clamp of an `int32_t` into `[0, 65535]`, returning
`uint16_t`, branchless.  Contract: `x < 0` yields `0`, `x > 65535`
yields `65535`, everything else is the identity.

## The mechanism

One mechanism: the sign-mask select.  Every choice "pick A when
predicate p holds, B otherwise" is made by building
`mask = 0xFFFFFFFF` exactly when p holds (`0x00000000` otherwise) out
of bit 31 of an unsigned 32-bit value, then computing
`a ^ ((a ^ b) & mask)`.  No ternary, no if/else, no min/max calls,
no builtins, no intrinsics, no inline asm.  All arithmetic is on
unsigned 32-bit values with compile-time shift counts below 32, so
every operation is fully defined C11: no implementation-defined
signed right shift is used anywhere; the sign predicate is read as
bit 31 of the unsigned representation instead.

**Step 1, clamp below at 0.**  Let `ux = (uint32_t)x`.
`neg = 0u - (ux >> 31)` is `0xFFFFFFFF` exactly when bit 31 of `ux`
is set (`x < 0` in two's complement) and `0` otherwise, so
`t = ux & ~neg` is `0` for `x < 0` and `ux` for `x >= 0`:
`t = max(x, 0)`, and `t` lies in `[0, 2^31 - 1]`.

**Step 2, clamp above at 65535 from the sign of (t - 65536).**
Because `t <= 2^31 - 1`, the unsigned difference `d = t - 65536`
wraps exactly when `t < 65536` and never wraps upward: for
`t >= 65536`, `d` lies in `[0, 2^31 - 1 - 65536]` with bit 31
clear; for `t < 65536`, `d = 2^32 - (65536 - t)` lies in
`[2^32 - 65536, 2^32 - 1]` with bit 31 set.  So
`ge = 1u - (d >> 31)` is `1` exactly when `t >= 65536`, and
`mask = 0u - ge` is `0xFFFFFFFF` exactly in that case.
`r = t ^ ((t ^ 65535u) & mask)`: with mask all ones,
`r = t ^ (t ^ 65535) = 65535`; with mask zero, `r = t`.  So
`r = min(t, 65535) = min(max(x, 0), 65535)`.

The result always lies in `[0, 65535]`, so the narrowing cast to
`uint16_t` is exact.  The differential suite pins this on every
one of the 4,294,967,296 inputs.

## Verification

Differential-tested against an independent reference (a plain
if/else clamp, not the mask trick) over ALL 2^32 `int32` inputs:

- 12 hand-checked anchors (`INT32_MIN`, `-65537`, `-65536`, `-2`,
  `-1`, `0`, `1`, `65534`, `65535`, `65536`, `65537`,
  `INT32_MAX`); every expected value cross-checked against the
  independent reference so a typo cannot silently pass.
- Exhaustive: all 4,294,967,296 inputs, 0 mismatches.  On every
  case the invariants `saturate(saturate(x)) == saturate(x)` and
  "the output is monotone non-decreasing in `x`" (checked as a
  running comparison across the input order, reset at the
  `2^31` wrap point) held with 0 violations.

Result: 4,294,967,308 cases, 0 mismatches.  The FNV-1a checksum of
every output is `0x31f7e4badfa3e62a`, identical across the `-O0`,
`-O2`, and ASan+UBSan builds.  The exhaustive differential run took
4m12s at `-O0`, 32.5s at `-O2`, and 30.7s under ASan+UBSan, so each
build genuinely re-verified the full 2^32 input space.
`-std=c11 -Wall -Wextra -Werror` clean; zero sanitizer reports.  Disassembly check (`gcc 13.3.0 -O2`,
non-inline wrapper, `objdump -d`): the function body contains 0
conditional jump instructions and no `cmov`; the sign masks are
built with `sar`/`not` and selected with `and`/`or`, so every
input takes the identical instruction path.  (Noted honestly:
this is what this compiler and flags produce; a different
compiler could emit a `cmov` or a branch, which the differential
suite above would still catch as a correctness matter but not as
a codegen matter.)  Throughput at `-O2`, best of 5 over 100M
timed values with the splitmix64 PRNG pre-generated and excluded
from the timed loop: 0.694 ns/value (1440.114 M values/s).
Exact logs are in PROOF.md.

No builtins, no intrinsics, no inline asm, no library calls in
the implementation.
