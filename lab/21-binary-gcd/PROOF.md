# PROOF.md: lab/21-binary-gcd

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then the three test binaries were run in turn. Output below is
the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_bgcd_o0 test_bgcd.c bgcd.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_bgcd_o2 test_bgcd.c bgcd.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_bgcd_asan test_bgcd.c bgcd.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
exhaustive_pairs=1048576 random_pairs=1000000 total=2048576 mismatches=0
checksum=11576187220487214191
random_sweep: pairs=1000000 ns_total=682445772 ns_per_pair=682.45
PASS
o0_exit=0
exhaustive_pairs=1048576 random_pairs=1000000 total=2048576 mismatches=0
checksum=11576187220487214191
random_sweep: pairs=1000000 ns_total=528205988 ns_per_pair=528.21
PASS
o2_exit=0
exhaustive_pairs=1048576 random_pairs=1000000 total=2048576 mismatches=0
checksum=11576187220487214191
random_sweep: pairs=1000000 ns_total=525304710 ns_per_pair=525.30
PASS
asan_exit=0
```

Timing caveats, stated honestly: the timed sweep includes the PRNG
step, the naive-Euclid oracle comparison, and the checksum
accumulation, so the ns/pair figures are the cost of one fully-checked
case, not the cost of one `bgcd32` call. The agreement of the
checksum across all three builds shows the tested behavior is
identical under `-O0`, `-O2`, and the sanitizers. Zero sanitizer
reports on the full 2,048,576-case run.
