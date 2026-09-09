# PROOF.md: lab/43-byte-permute

Environment: gcc 13.3.0 (Ubuntu), x86_64.

## Build

```sh
$ make clean && make
rm -f test_permute test_permute_asan test_permute_o0
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_permute test_permute.c
build_exit=0
```

Zero warnings under -Wall -Wextra -Werror.

## Runs

### -O2 (`./test_permute`)

```
cases=1065536
mismatches=0
invariant_failures=0
fnv1a=f75c74855e39dfd5
bench_time=0.009 s
throughput_ns_per_value=4.424
throughput_acc=647458ecb4001000
exit_code=0
```

### -O0 (`make opt0`)

```
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_permute_o0 test_permute.c
./test_permute_o0
cases=1065536
mismatches=0
invariant_failures=0
fnv1a=f75c74855e39dfd5
bench_time=0.066 s
throughput_ns_per_value=32.787
throughput_acc=647458ecb4001000
exit_code=0
```

### ASan+UBSan (`make sanitize`)

```
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_permute_asan test_permute.c
./test_permute_asan
cases=1065536
mismatches=0
invariant_failures=0
fnv1a=f75c74855e39dfd5
bench_time=0.014 s
throughput_ns_per_value=7.191
throughput_acc=647458ecb4001000
exit_code=0
```

## What the numbers mean

- `cases` = 65,536 exhaustive 16-bit inputs + 1,000,000 fixed-seed
  splitmix64 64-bit values, each checked 4 ways: `permute64` vs the
  byte-loop oracle, `inv_permute64` vs its oracle, and both directions
  of the round-trip invariant `inv(permute(x)) == x` and
  `permute(inv(x)) == x`. All 1,065,536 cases passed with 0 mismatches
  and 0 invariant failures.
- `fnv1a` = FNV-1a over every permute and inverse result. Identical
  (f75c74855e39dfd5) across -O0, -O2, and ASan+UBSan builds, so the
  observable behavior does not depend on the optimization level or
  instrumentation.
- `throughput_ns_per_value` = measured over 2,000,000 permute/inverse
  calls on random inputs at -O2: 4.424 ns per call.
- The sanitizer run reported nothing; no address or undefined-behavior
  findings.
- The PRNG seed is fixed (0x123456789ABCDEF0), so any rerun checks the
  same 1,000,000 values.
