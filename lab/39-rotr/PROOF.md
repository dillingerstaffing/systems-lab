<!-- PROOF-HEADER
Checks: 4194304
Mismatches: 0
Throughput: rotr 2.73 ns/value, rotl 2.66 ns/value at -O2 (100M values; timed loop includes one splitmix64 PRNG step per value)
-->

# lab/39-rotr: proof log

`make clean && make all` on 2026-09-09 (gcc 13.3.0, Ubuntu 24.04).
The build succeeded on the first attempt with zero warnings.

```
=== BUILD LOG (make all) ===
rm -f test_rotr_o0 test_rotr_o2 test_rotr_asan
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_rotr_o0 test_rotr.c rotr.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -DMEASURE -O2 -o test_rotr_o2 test_rotr.c rotr.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_rotr_asan test_rotr.c rotr.c
BUILD_EXIT=0
```

Zero warnings at `-Wall -Wextra -Werror` on all three configs. The
ASan+UBSan binary ran the full exhaustive loop (all 65536 inputs times
32 rotation amounts, 4,194,304 total checks) with no sanitizer reports.

```
=== RUN: ./test_rotr_o0 ===
rotr differential (impl vs bit-loop ref): 2097152 checks, 0 mismatches
invariant rotr(rotl(x, r), r) == x: 2097152 checks, 0 mismatches
FNV-1a checksum: 501688248194884901
EXIT=0
=== RUN: ./test_rotr_o2 ===
rotr differential (impl vs bit-loop ref): 2097152 checks, 0 mismatches
invariant rotr(rotl(x, r), r) == x: 2097152 checks, 0 mismatches
FNV-1a checksum: 501688248194884901
rotr throughput: 2.73 ns/value (100M values)
rotl throughput: 2.66 ns/value (100M values)
EXIT=0
=== RUN: ./test_rotr_asan ===
rotr differential (impl vs bit-loop ref): 2097152 checks, 0 mismatches
invariant rotr(rotl(x, r), r) == x: 2097152 checks, 0 mismatches
FNV-1a checksum: 501688248194884901
EXIT=0
```

Additional timing runs of the `-O2` binary for stability:

```
rotr throughput: 2.77 ns/value (100M values)
rotl throughput: 2.65 ns/value (100M values)
rotr throughput: 2.65 ns/value (100M values)
rotl throughput: 2.65 ns/value (100M values)
```

Both rotations sit at roughly 2.7 ns/value. The timed loop includes one
splitmix64 PRNG step per value, so these numbers are a ceiling on the raw
rotation rate; the PRNG cost is included honestly rather than subtracted.
The checksum is identical across all three optimization/sanitizer builds,
confirming the computed results do not depend on the build flags.
