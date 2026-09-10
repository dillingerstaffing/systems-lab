<!-- PROOF-HEADER
Throughput: 3.68-4.92 ns/step at -O2 (two 65,535-step walks; timed loop includes visited-array store, checks, bookkeeping)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/20-xorshift-period

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then `make run`. Output below is the genuine build log and the
genuine run output, captured verbatim.

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_xorshift_period_o0 test_xorshift_period.c xorshift16.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_xorshift_period_o2 test_xorshift_period.c xorshift16.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_xorshift_period_asan test_xorshift_period.c xorshift16.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
./test_xorshift_period_o0
fixed_point_zero=ok
seed_0x0001_prefix=8181 6021 e999 2e0b b59e d9a3 2f27 45f9
seed=0x0001 steps=65535 expected=65535 ns=1370378 ns_per_step=20.91
seed=0xbeef steps=65535 expected=65535 ns=1318030 ns_per_step=20.11
PASS
./test_xorshift_period_o2
fixed_point_zero=ok
seed_0x0001_prefix=8181 6021 e999 2e0b b59e d9a3 2f27 45f9
seed=0x0001 steps=65535 expected=65535 ns=322705 ns_per_step=4.92
seed=0xbeef steps=65535 expected=65535 ns=240892 ns_per_step=3.68
PASS
./test_xorshift_period_asan
fixed_point_zero=ok
seed_0x0001_prefix=8181 6021 e999 2e0b b59e d9a3 2f27 45f9
seed=0x0001 steps=65535 expected=65535 ns=319119 ns_per_step=4.87
seed=0xbeef steps=65535 expected=65535 ns=446150 ns_per_step=6.81
PASS
```

All three binaries exit 0. ASan and UBSan report no issues, so the
65,536-entry visited array is indexed in bounds on all 131,070 steps
and every shift is defined.

## What the numbers mean

- Seed 0x0001 returns to 0x0001 after exactly 65,535 steps, with no
  state repeated before the return. The visited array held 65,535
  distinct marks at the end, so all 65,535 nonzero 16-bit states were
  visited exactly once: the walk is a single full cycle of length
  2^16 - 1. The same holds for the unrelated seed 0xBEEF, which
  confirms the invariant that every nonzero seed rides the same one
  cycle (the step function is a permutation of the state space, so
  cycles partition it, and a full-length cycle from one seed leaves
  nothing for any other).
- The state 0 was never visited from a nonzero seed, and
  `xorshift16_step(0) == 0` was verified directly. Zero is a fixed
  point of the recurrence, so the generator provably cannot enter or
  leave it; that is why the maximal period is 2^16 - 1 and not 2^16.
- The (7, 9, 8) triple is a documented maximal-period choice for
  16-bit xorshift; the exhaustive walk above re-verifies that claim
  against ground truth rather than trusting the citation.
- The first 8 outputs from seed 1 are printed so the stream is
  concrete and re-checkable by hand: 1 -> 0x8181 (1 ^ 0x80 ^ 0x8100),
  then 0x6021, 0xe999, 0x2e0b, 0xb59e, 0xd9a3, 0x2f27, 0x45f9. All
  three binaries (including the sanitizer build) produced identical
  prefixes, so the generator is bit-for-bit reproducible across
  optimization levels.
- Timing: CLOCK_MONOTONIC over the full 65,535-step walk. Honest
  caveat: the timed loop includes the visited-array store, the
  zero/repeat checks, and the loop bookkeeping per step, so
  ns_per_step is a ceiling for the combined loop, not a pure
  generator measurement: 20.11-20.91 ns/step at -O0, 3.68-4.92
  ns/step at -O2. The walk is fast enough that the measurement was
  dominated by 65,535 iterations, not by timer resolution.
