# PROOF.md: lab/30-memmove-overlap

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then `make run`. Output below is the genuine build log and the
genuine run output, captured verbatim.

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_memmove_o0 test_memmove.c memmove30.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_memmove_o2 test_memmove.c memmove30.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_memmove_asan test_memmove.c memmove30.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
./test_memmove_o0
total_cases=8385 mismatches=0 checksum=13318231107937211255
directed backward: mine_matches=1 naive_corrupts=1 naive_got=ABCDABCD
directed forward: matches_libc=1
bench backward_overlap 4MiB x 200: my_memmove=445.8 MiB/s libc_memmove=19733.8 MiB/s acc=48000
PASS
./test_memmove_o2
total_cases=8385 mismatches=0 checksum=13318231107937211255
directed backward: mine_matches=1 naive_corrupts=1 naive_got=ABCDABCD
directed forward: matches_libc=1
bench backward_overlap 4MiB x 200: my_memmove=1203.0 MiB/s libc_memmove=44132.1 MiB/s acc=48000
PASS
./test_memmove_asan
total_cases=8385 mismatches=0 checksum=13318231107937211255
directed backward: mine_matches=1 naive_corrupts=1 naive_got=ABCDABCD
directed forward: matches_libc=1
bench backward_overlap 4MiB x 200: my_memmove=306.4 MiB/s libc_memmove=1233.9 MiB/s acc=48000
PASS
```

Run exit codes: 0, 0, 0.

The differential set is exhaustive over the stated space: 65 sizes x
129 offsets = 8,385 cases, 0 mismatches, and the checksum is identical
across all three builds, so the result does not depend on optimization
or instrumentation. The directed case shows the mechanism directly:
the always-forward naive copy corrupts to `ABCDABCD` while
`my_memmove` matches the oracle.

Benchmark notes: throughput varies with machine load (measured runs at
-O2 ranged roughly 1.0-1.7 GiB/s for `my_memmove` and 37-53 GiB/s for
libc `memmove`; the run above showed 1203.0 vs 44132.1 MiB/s). The gap
is the documented cost of the byte-at-a-time copy: no word-aligned fast
path was included.
