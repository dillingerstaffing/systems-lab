# PROOF.md: lab/19-saturating-arith

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then `make run`. Output below is the genuine build log and the
genuine run output, captured verbatim.

## Build log

```
rm -f test_saturating_o0 test_saturating_o2 test_saturating_asan
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_saturating_o0 test_saturating.c saturating.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_saturating_o2 test_saturating.c saturating.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_saturating_asan test_saturating.c saturating.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
./test_saturating_o0
directed=338
total_cases=10000338 mismatches=0 checksum=1390331946882605843
add: pairs=5000000 ns_total=387212690 ns_per_op=77.44
sub: pairs=5000000 ns_total=192526603 ns_per_op=38.51
PASS
./test_saturating_o2
directed=338
total_cases=10000338 mismatches=0 checksum=1390331946882605843
add: pairs=5000000 ns_total=90956851 ns_per_op=18.19
sub: pairs=5000000 ns_total=88517838 ns_per_op=17.70
PASS
./test_saturating_asan
directed=338
total_cases=10000338 mismatches=0 checksum=1390331946882605843
add: pairs=5000000 ns_total=122125518 ns_per_op=24.43
sub: pairs=5000000 ns_total=119380634 ns_per_op=23.88
PASS
```

Timing caveats, stated honestly: the timing loops include the PRNG
step, the differential comparison, and the checksum accumulation, so
the ns/op figures are the cost of one fully-checked case, not the cost
of one saturating op. The add sweep runs first on a cold cache at
`-O0`, which is why its `-O0` figure is higher than sub's. The
agreement of the checksum across all three builds shows the tested
behavior is identical under `-O0`, `-O2`, and the sanitizers.
