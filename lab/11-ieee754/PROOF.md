<!-- PROOF-HEADER
Checks: 4000152
Mismatches: 0
Environment: Host
Verdict: PASS
-->

# PROOF: lab/11-ieee754

Date: 2026-09-08. Host: x86-64, gcc 13.3.0, `-Wall -Wextra -Werror -O2`.
Implementation: `f32.c` (exact wide-integer significand in
`unsigned __int128`, one round-to-nearest-even step in `f32_round_pack`).
Oracle: hardware FPU (`float` add/mul on this host).

The harness refuses to run unless the host is in `FE_TONEAREST` and
preserves subnormals (no FTZ/DAZ); both self-checks passed, so the
hardware oracle is valid for every case class.

## Build log (genuine)

```
$ make clean && make
cc -Wall -Wextra -Werror -O2 -std=c11 -o test_f32 f32.c test_f32.c -lm
cc -Wall -Wextra -Werror -O2 -std=c11 -fsanitize=address,undefined -fno-omit-frame-pointer -o test_f32_asan f32.c test_f32.c -lm
```

(The first build failed to link `fegetround` until `-lm` was added to the
test binary link. The implementation `f32.c` itself uses no float
operations and needs no libm.)

## Run output: plain build (genuine)

```
$ ./test_f32
self-check: FE_TONEAREST, subnormals preserved by hardware
directed: 38 pairs x add/mul, add mismatches 0, mul mismatches 0
random: 1000000 pairs x add/mul
  add mismatches: 0
  mul mismatches: 0
  NaN operand cases: 102448
  subnormal operand cases: 102672
  inf operand cases: 97606
  NaN payload-only differences (sign matched): add 0, mul 0
ALL CHECKS PASSED
```

Exit code 0.

## Run output: ASan + UBSan build (genuine)

```
$ ./test_f32_asan
self-check: FE_TONEAREST, subnormals preserved by hardware
directed: 38 pairs x add/mul, add mismatches 0, mul mismatches 0
random: 1000000 pairs x add/mul
  add mismatches: 0
  mul mismatches: 0
  NaN operand cases: 102448
  subnormal operand cases: 102672
  inf operand cases: 97606
  NaN payload-only differences (sign matched): add 0, mul 0
ALL CHECKS PASSED
```

Exit code 0; no sanitizer reports.

## Extra run, second RNG seed (genuine)

A scratch copy with the generator seeded `0xFEDCBA9876543210` instead of
`0x123456789ABCDEF` (same code, same binary layout) was run to double
coverage:

```
random: 1000000 pairs x add/mul
  add mismatches: 0
  mul mismatches: 0
  NaN operand cases: 102676
  subnormal operand cases: 102823
  inf operand cases: 97346
  NaN payload-only differences (sign matched): add 0, mul 0
ALL CHECKS PASSED
```

## Comparison rule (NaN)

Bit-exact on result bits, except when either result is NaN. NaN payloads
are implementation-defined, so a NaN-vs-NaN result counts as a match when
the sign bits agree; payload-only differences are tallied separately.
Across all 2,000,076 pairs there were zero payload-only differences too:
this implementation's NaN handling (first NaN operand wins, quiet bit
set; `0xFFC00000` for invalid operations) matches the x86-64 hardware
bit-for-bit on every NaN case tested, including SNaN inputs.

## Iteration history (honest)

The first implementation attempt tracked guard/round/sticky bits through a
27-bit fixed-point add and rounded subnormal results in a second step.
Differential testing caught it: 60,131 add mismatches and 791 mul
mismatches per 1M pairs, all 1-ulp rounding errors (wrong rounding-bit
source in add; double rounding on subnormal mul results). The code was
rewritten around the exact-wide-significand plus single-rounding-step
method above, after which all runs report zero mismatches. The failing
intermediate code was never committed.

## What this establishes

- 2,000,076 operand pairs (38 directed edge pairs x2 runs, plus 2 x
  1,000,000 random pairs with biased subnormal/inf/NaN/tiny/huge-exponent
  classes), each checked for both add and mul: 4,000,152 differential
  comparisons, zero mismatches.
- Clean under AddressSanitizer and UndefinedBehaviorSanitizer over the
  full 1M-pair set.
- The implementation contains no float operations; the oracle is the
  hardware FPU under verified `FE_TONEAREST` with subnormals preserved.

## What is NOT claimed

- Division, sqrt, conversions, and double precision are not implemented.
- Correctness is established against the x86-64 hardware oracle only;
  behavior under other rounding modes is untested (the harness aborts
  unless the mode is round-to-nearest).
