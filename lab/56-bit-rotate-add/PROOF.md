# PROOF.md: lab/56-bit-rotate-add

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then the three test binaries were run in turn. Output below is
the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_rot_add_o0 test_rot_add.c rot_add.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -DPERF_TEST -O2 -o test_rot_add_o2 test_rot_add.c rot_add.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_rot_add_asan test_rot_add.c rot_add.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
directed: 10/10 OK
differential: amounts=64 values_per_amount=1000000 cases=64000000 rotl_mism=0 ident_mism=0 rotadd_mism=0
checksum=11571239451276328976
PASS
o0_exit=0
directed: 10/10 OK
differential: amounts=64 values_per_amount=1000000 cases=64000000 rotl_mism=0 ident_mism=0 rotadd_mism=0
checksum=11571239451276328976
throughput: values=100000000 ns_total=1051443818 ns_per_value=10.51 sink=1018120567130385963
PASS
o2_exit=0
directed: 10/10 OK
differential: amounts=64 values_per_amount=1000000 cases=64000000 rotl_mism=0 ident_mism=0 rotadd_mism=0
checksum=11571239451276328976
PASS
asan_exit=0
```

The FNV-1a checksum `11571239451276328976` is identical across the
`-O0`, `-O2`, and ASan+UBSan builds. The ASan+UBSan build ran the full
64,000,000-case sweep with every shift in both the implementation and
the per-bit reference under the sanitizers and reported no errors,
which means no shift count ever reached 64 and no other undefined
behavior occurred.

Note on test development, recorded honestly: the first run failed one
directed case, `rotl(x, 63)`. The 64,000,000-case differential sweep in
that same run reported 0 mismatches between the implementation and the
independent per-bit reference, including at k = 63, which showed the
implementation was correct and the hand-computed expected value was
wrong: it had dropped bit 0 of x, which rotates into bit 63
(`0x0091A2B3C4D5E6F7` corrected to `0x8091A2B3C4D5E6F7`). The test was
fixed and all three builds rerun from clean; the output above is from
the rerun.
