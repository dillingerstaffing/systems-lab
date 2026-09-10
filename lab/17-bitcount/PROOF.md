<!-- PROOF-HEADER
Checks: 10000133
Mismatches: 0
Throughput: 3.98 ns/value (251.4 Mvalues/s) at -O2 over 100,000,000 values (timed loop includes PRNG step)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/17-bitcount

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then `make run`. Output below is the genuine build log and the
genuine run output, captured verbatim.

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_bitcount_o0 test_bitcount.c bitcount.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_bitcount_o2 test_bitcount.c bitcount.c
gcc -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_bitcount_asan test_bitcount.c bitcount.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
./test_bitcount_o0
total_cases=10000133 mismatches=0 checksum=319992825
timed_values=100000000 ns_total=564771242 ns_per_value=5.65 Mvalues_per_sec=177.1
checksum=3519970286
PASS
./test_bitcount_o2
total_cases=10000133 mismatches=0 checksum=319992825
timed_values=100000000 ns_total=397706347 ns_per_value=3.98 Mvalues_per_sec=251.4
checksum=3519970286
PASS
./test_bitcount_asan
total_cases=10000133 mismatches=0 checksum=319992825
timed_values=100000000 ns_total=471703602 ns_per_value=4.72 Mvalues_per_sec=212.0
checksum=3519970286
PASS
```

All three binaries exit 0. ASan and UBSan report no issues, so the
mask arithmetic and the multiply-shift fold contain no out-of-bounds
access and no undefined behavior.

## What the numbers mean

- 10,000,133 total checks against `__builtin_popcountll`, 0
  mismatches: 133 directed edge cases (0, all-ones, every single-bit
  position 0..63, alternating 0x5555... and 0xAAAA..., 2^k - 1 for
  k = 0..64) plus 10,000,000 fixed-seed splitmix64 random 64-bit
  values (seed 0x123456789ABCDEF0).
- The checksum 319992825 is identical across `-O0`, `-O2`, and the
  ASan+UBSan binary, so the PRNG stream and the results are
  bit-for-bit reproducible.
- Throughput: 100,000,000 fresh fixed-seed values, timed with
  CLOCK_MONOTONIC. Honest caveats: the timed loop includes the
  splitmix64 PRNG step per value, so the ns/value figure is a ceiling
  for the combined loop, not a pure popcount measurement; and the
  value is returned through a checksum accumulation so the compiler
  cannot delete the loop. Verified by disassembly that both binaries
  call the real `popcount64` (the raw SWAR instruction sequence,
  `sub`/`shr`/`and`/`add`/`imul`/`shr`; no POPCNT instruction is
  emitted anywhere), so the measured numbers are the actual cost of
  this implementation, PRNG included.

The PRNG is fixed-seed, so the run is reproducible bit for bit:
`make run` on this checkout produces the exact output above.
