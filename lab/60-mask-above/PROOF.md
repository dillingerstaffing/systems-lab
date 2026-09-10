<!-- PROOF-HEADER
Checks: 65000000
Mismatches: 0
Checksum: cf623e770a179ce5
Throughput: 3.379 ns per value at -O2
Environment: Host
-->
# PROOF.md: lab/60-mask-above

Environment: gcc 13.3.0 (Ubuntu), x86_64.

`mask_above64(n)` returns the n high bits of a 64-bit word set, built from
the identity `~0ULL << (64 - n)`. The n = 0 case returns 0 through a ternary
branch, so the shift operand (64 - n) is never 64; n = 64 shifts by 0 and
returns ~0ULL. No library popcount or builtin is used anywhere.

## Build

```
$ make clean && make
rm -f test_mask test_mask_asan test_mask_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_mask test_mask.c
build_exit=0
```

Zero warnings under -Wall -Wextra -Werror.

## Runs

### -O2 (`./test_mask`)

```
cases=65000000
mismatches=0
invariant_failures=0
random_failures=0
fnv1a=cf623e770a179ce5
verification_time=3.018 s
throughput_ns_per_value=3.379
throughput_acc=442168ec09edbb38
run_exit=0
```

### -O0 (`make opt0`)

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_mask_o0 test_mask.c
./test_mask_o0
cases=65000000
mismatches=0
invariant_failures=0
random_failures=0
fnv1a=cf623e770a179ce5
verification_time=14.157 s
throughput_ns_per_value=8.388
throughput_acc=442168ec09edbb38
```

### ASan+UBSan (`make sanitize`)

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_mask_asan test_mask.c
./test_mask_asan
cases=65000000
mismatches=0
invariant_failures=0
random_failures=0
fnv1a=cf623e770a179ce5
verification_time=6.100 s
throughput_ns_per_value=9.130
throughput_acc=442168ec09edbb38
```

The sanitizer run printed no reports: zero ASan/UBSan findings. In
particular UBSan did not flag the n = 0 edge case, confirming no 64-bit
shift by 64 executes there.

## What was measured

- 65,000,000 differential cases (65 widths x 1,000,000 fixed-seed
  splitmix64 draws with seed 0x123456789ABCDEF0): `mask_above64(n)`
  vs the independent per-bit reference that sets each of the n high
  bits one at a time. 0 mismatches.
- Invariants on all 65,000,000 cases: `popcount(mask) == n` (naive
  bit-stripping popcount, no builtins) and
  `mask == 0 || (mask | (mask - 1)) == ~0ULL` (contiguity of the
  high-bit field). 0 failures.
- Random property on all 65,000,000 cases: masking a random 64-bit
  word with the mask leaves no low bits standing, i.e.
  `(draw & mask) & low_mask(64 - n) == 0`. 0 failures.
- FNV-1a checksum over all 65,000,000 results: `cf623e770a179ce5`,
  identical across -O0, -O2, and ASan+UBSan builds.
- Throughput at -O2: 3.379 ns per value over 100,000,000 values, each
  a splitmix64 draw masked by `mask_above64(i % 65)` with the results
  xor-folded into a printed accumulator so the calls are not optimized
  away. The PRNG step is included in the timed loop, so this is a
  ceiling on the mask cost alone.

## Limits of verification

- The n = 0 case never executes a shift by 64 only because the ternary
  in mask.h takes the constant branch; the disassembly was not
  inspected this run, so the claim rests on the source-level guard and
  the clean UBSan run, not on machine code.
- Throughput was measured on one x86_64 host (gcc 13.3.0); the number
  is not portable and includes the PRNG step.
