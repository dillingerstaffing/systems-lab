# PROOF.md: lab/55-zero-run

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then the three test binaries were run in turn. Output below is
the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_zero_run_o0 test_zero_run.c zero_run.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_zero_run_o2 test_zero_run.c zero_run.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_zero_run_asan test_zero_run.c zero_run.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
directed: zeros=64 ones=0 alt=1 hi=62 half=32
singlebit: 64 positions OK, singleclear: 64 positions OK
exhaustive_16bit=65536 random_64bit=1000000 total=1065669 mismatches=0
checksum=9935966997446426842
throughput: values=100000000 ns_total=3902033601 ns_per_value=39.02 sink=535639027
PASS
o0_exit=0
directed: zeros=64 ones=0 alt=1 hi=62 half=32
singlebit: 64 positions OK, singleclear: 64 positions OK
exhaustive_16bit=65536 random_64bit=1000000 total=1065669 mismatches=0
checksum=9935966997446426842
throughput: values=100000000 ns_total=3146892003 ns_per_value=31.47 sink=535639027
PASS
o2_exit=0
directed: zeros=64 ones=0 alt=1 hi=62 half=32
singlebit: 64 positions OK, singleclear: 64 positions OK
exhaustive_16bit=65536 random_64bit=1000000 total=1065669 mismatches=0
checksum=9935966997446426842
throughput: values=100000000 ns_total=3621529527 ns_per_value=36.22 sink=535639027
PASS
asan_exit=0
```

## What the numbers mean

- 1,065,669 differential checks, 0 mismatches: 5 directed cases
  (`0` -> 64, all-ones -> 0, `0xAAAAAAAAAAAAAAAA` -> 1,
  `0x8000000000000001` -> 62, `0x00000000FFFFFFFF` -> 32), 64
  single-set-bit positions, 64 single-clear-bit positions, all 65,536
  16-bit values, and 1,000,000 fixed-seed (splitmix64,
  seed `0x243F6A8885A308D3`) full-range 64-bit values, each compared
  against an independent naive bit-walking oracle.
- The FNV-1a checksum `9935966997446426842` is identical across the
  `-O0`, `-O2`, and ASan+UBSan builds; zero sanitizer reports.
- Throughput at `-O2`: 31.47 ns/value over 100,000,000 timed values
  (sink sum 535,639,027, average longest run 5.36 per value).
