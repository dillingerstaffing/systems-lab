# PROOF.md: lab/27-fixed-point

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean`, then
`make`, then `make run`. Output below is the genuine build log and the
genuine run output, captured verbatim.

## Build log

```
rm -f test_fixed_point_o0 test_fixed_point_o2 test_fixed_point_asan
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_fixed_point_o0 test_fixed_point.c fixed_point.c -lm
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_fixed_point_o2 test_fixed_point.c fixed_point.c -lm
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_fixed_point_asan test_fixed_point.c fixed_point.c -lm
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
./test_fixed_point_o0
directed=1352
mul: pairs=5000000 ns_total=549468671 ns_per_op=109.89
add: pairs=5000000 ns_total=253289823 ns_per_op=50.66
mul_in_range=3677800 mul_wrap=1322876 mul_ties=507 add=5000676
total_cases=10001352 mismatches=0 checksum=9619856039744915734
PASS
./test_fixed_point_o2
directed=1352
mul: pairs=5000000 ns_total=193922851 ns_per_op=38.78
add: pairs=5000000 ns_total=147966935 ns_per_op=29.59
mul_in_range=3677800 mul_wrap=1322876 mul_ties=507 add=5000676
total_cases=10001352 mismatches=0 checksum=9619856039744915734
PASS
./test_fixed_point_asan
directed=1352
mul: pairs=5000000 ns_total=541758240 ns_per_op=108.35
add: pairs=5000000 ns_total=167394610 ns_per_op=33.48
mul_in_range=3677800 mul_wrap=1322876 mul_ties=507 add=5000676
total_cases=10001352 mismatches=0 checksum=9619856039744915734
PASS
```

Timing caveats, stated honestly: the timing loops include the PRNG
step, the independent oracle comparison, the double-reference check,
and the checksum accumulation, so the ns/op figures are the cost of
one fully-checked case, not the cost of one fixed-point op. Timings
vary a few percent run to run; case counts, mismatch counts, and the
checksum are exactly reproducible (fixed seed), and the checksum
`9619856039744915734` is identical across `-O0`, `-O2`, and
ASan+UBSan, showing the tested behavior is identical under all three
builds. Of the 5,000,000 random mul pairs, 3,677,800 took the in-range
path (checked within 1 ulp of the double reference), 1,322,876 took
the documented wraparound path (checked against the wrap oracle, with
the double product confirmed out of range), and 507 exact rounding
ties were observed and confirmed to round away from zero.
