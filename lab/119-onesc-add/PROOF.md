<!-- PROOF-HEADER
Checks: 10065536
Mismatches: 0
Checksum: 12261447224700235540
Throughput: 3.14 ns/pair at -O2 over 100,000,000 pairs (best of 5 rounds; timed loop includes splitmix64 PRNG step)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/119-onesc-add

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then the three test binaries were run in turn. Output below is
the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_onesc_add_o0 test_onesc_add.c onesc_add.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_onesc_add_o2 test_onesc_add.c onesc_add.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_onesc_add_asan test_onesc_add.c onesc_add.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
edge_pins: 4 checked, onesc_add(0xFFFF, 0xFFFF) = 0xFFFF
directed_65536=65536 random_pairs=10000000 total=10065536 mismatches=0
checksum=12261447224700235540
throughput: pairs=100000000 best_of_5 ns_total=1092210843 ns_per_pair=10.92 sink=16386899863180
PASS
o0_exit=0
edge_pins: 4 checked, onesc_add(0xFFFF, 0xFFFF) = 0xFFFF
directed_65536=65536 random_pairs=10000000 total=10065536 mismatches=0
checksum=12261447224700235540
throughput: pairs=100000000 best_of_5 ns_total=314361173 ns_per_pair=3.14 sink=16386899863180
PASS
o2_exit=0
edge_pins: 4 checked, onesc_add(0xFFFF, 0xFFFF) = 0xFFFF
directed_65536=65536 random_pairs=10000000 total=10065536 mismatches=0
checksum=12261447224700235540
throughput: pairs=100000000 best_of_5 ns_total=566273240 ns_per_pair=5.66 sink=16386899863180
PASS
asan_exit=0
```

What the numbers mean: 10,065,536 differential checks against the
loop-fold oracle (RFC 1071 form, 32-bit accumulation with repeated
high-half folding, sharing no arithmetic with the tested code),
0 mismatches. The all-ones identity is pinned:
`onesc_add(0xFFFF, 0xFFFF) == 0xFFFF`. The FNV-1a checksum of every
verified result is identical (`12261447224700235540`) across
`-O0`, `-O2`, and ASan+UBSan, and the benchmark sink total is
identical too, so the tested behavior does not depend on the
optimization level or instrumentation. Zero sanitizer reports on the
full run.

Timing caveat, stated honestly: the timed 100M-pair loop includes
one splitmix64 PRNG step per pair, so the ns/pair figures are the
cost of one generate-and-add case, not one bare `onesc_add` call.
