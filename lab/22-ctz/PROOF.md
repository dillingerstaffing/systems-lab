# PROOF.md: lab/22-ctz

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then the three test binaries were run in turn. Output below is
the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_ctz_o0 test_ctz.c ctz.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_ctz_o2 test_ctz.c ctz.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_ctz_asan test_ctz.c ctz.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
debruijn_identity: 32 powers checked, distinct_5bit_keys=32
exhaustive_16bit=65536 random_32bit=2000000 total=2065536 mismatches=0
checksum=7337941371035615009
throughput: values=100000000 ns_total=512654439 ns_per_value=5.13 sink=99978375
PASS
o0_exit=0
debruijn_identity: 32 powers checked, distinct_5bit_keys=32
exhaustive_16bit=65536 random_32bit=2000000 total=2065536 mismatches=0
checksum=7337941371035615009
throughput: values=100000000 ns_total=342783055 ns_per_value=3.43 sink=99978375
PASS
o2_exit=0
debruijn_identity: 32 powers checked, distinct_5bit_keys=32
exhaustive_16bit=65536 random_32bit=2000000 total=2065536 mismatches=0
checksum=7337941371035615009
throughput: values=100000000 ns_total=488516923 ns_per_value=4.89 sink=99978375
PASS
asan_exit=0
```

Timing caveats, stated honestly: the timed 100M-value loop includes
the PRNG step per value, so the ns/value figures are the cost of one
generate-and-count case, not one bare `ctz32` call, and they vary
about +-0.3 ns run to run on this machine. The agreement of the
checksum across all three builds shows the tested behavior is
identical under `-O0`, `-O2`, and the sanitizers. Zero sanitizer
reports on the full 2,065,536-case run.

A note on the development of the test itself: the first version of
the naive reference looped `while ((x & 1u) == 0u)` with no bound, so
`ref_ctz32(0u)` spun forever on the exhaustive sweep's very first
case. That was a bug in the test harness, not in `ctz32`; it was
fixed by bounding the walk to 32 steps (which also pins the
`ctz(0) = 32` contract), and the numbers above are from the fixed
harness.

Disassembly of `ctz32` at `-O2` (gcc 13.3.0, x86_64), shown here
because it was actually inspected, not assumed:

```
0000000000000000 <ctz32>:
   0:	f3 0f 1e fa         	endbr64
   4:	b8 20 00 00 00      	mov    eax,0x20
   9:	85 ff               	test   edi,edi
   b:	74 1a               	je     27 <ctz32+0x27>
   d:	89 f8               	mov    eax,edi
   f:	48 8d 15 00 00 00 00 	lea    rdx,[rip+0x0]        # 16 <ctz32+0x16>
  16:	f7 d8               	neg    eax
  18:	21 f8               	and    eax,edi
  1a:	69 c0 31 b5 7c 07   	imul   eax,eax,0x77cb531
  20:	c1 e8 1b            	shr    eax,0x1b
  23:	0f b6 04 02         	movzx  eax,BYTE PTR [rdx+rax*1]
  27:	c3                  	ret
```

The compiler kept the multiply-and-table construction exactly as
written (the `lea` loads the de Bruijn table base, `neg`/`and`
isolate the lowest set bit, `imul` multiplies by the constant,
`shr 27` takes the top 5 bits, `movzx` does the table lookup); it
did not substitute a hardware `tzcnt`.
