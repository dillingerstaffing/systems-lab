<!-- PROOF-HEADER
Checks: 5194304
Mismatches: 0
Checksum: 10430319035230998775
Throughput: 6.199 ns/value at -O2 over 20,000,000 values (best of 5 trials; timed loop includes PRNG step)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/127-sat-shl

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make clean` then
`make`, then `make run`, then `make disasm`. Output below is the
genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_satshl_o0 test_satshl.c satshl.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_satshl_o2 test_satshl.c satshl.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_satshl_asan test_satshl.c satshl.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
sweep_k=0..63 sweep_x=0..65535 random_pairs=1000000 total=5194304 mismatches=0
checksum=10430319035230998775
trial 0: ns_per_value=22.887
trial 1: ns_per_value=22.202
trial 2: ns_per_value=19.045
trial 3: ns_per_value=18.971
trial 4: ns_per_value=26.251
throughput: best_of_5 trials values=20000000 ns_per_value=18.971 sink=8804416366858687454
PASS
o0_exit=0
sweep_k=0..63 sweep_x=0..65535 random_pairs=1000000 total=5194304 mismatches=0
checksum=10430319035230998775
trial 0: ns_per_value=8.397
trial 1: ns_per_value=6.593
trial 2: ns_per_value=6.420
trial 3: ns_per_value=6.783
trial 4: ns_per_value=6.199
throughput: best_of_5 trials values=20000000 ns_per_value=6.199 sink=8804416366858687454
PASS
o2_exit=0
sweep_k=0..63 sweep_x=0..65535 random_pairs=1000000 total=5194304 mismatches=0
checksum=10430319035230998775
trial 0: ns_per_value=10.432
trial 1: ns_per_value=10.405
trial 2: ns_per_value=10.377
trial 3: ns_per_value=10.418
trial 4: ns_per_value=10.471
throughput: best_of_5 trials values=20000000 ns_per_value=10.377 sink=8804416366858687454
PASS
asan_exit=0
```

The checksum is identical across `-O0`, `-O2`, and ASan+UBSan, and
the ASan+UBSan run reports no sanitizer errors (exit code 0), so no
shift in the construction is ever out of range.

## Branchlessness check (`make disasm`)

The Makefile compiles `satshl.c` standalone at `-O2`, disassembles
`sat_shl64`, and fails the build if any conditional jump appears.
The scan counts `j<cc>` mnemonics excluding unconditional `jmp`:

```
0000000000000000 <sat_shl64>:
   0:	f3 0f 1e fa         		endbr64
   4:	48 89 fa             		mov    %rdi,%rdx
   7:	89 f7                		mov    %esi,%edi
   9:	b9 3f 00 00 00       		mov    $0x3f,%ecx
   e:	83 e7 3f             		and    $0x3f,%edi
  11:	48 85 d2             		test   %rdx,%rdx
  14:	49 89 d0             		mov    %rdx,%r8
  17:	0f 95 c0             		setne  %al
  1a:	29 f9                		sub    %edi,%ecx
  1c:	49 d3 e8             		shr    %cl,%r8
  1f:	4c 89 c1             		mov    %r8,%rcx
  22:	48 d1 e9             		shr    $1,%rcx
  25:	0f 95 c1             		setne  %cl
  28:	c1 ee 06             		shr    $0x6,%esi
  2b:	0f 44 c1             		cmove  %ecx,%eax
  2e:	89 f9                		mov    %edi,%ecx
  30:	48 d3 e2             		shl    %cl,%rdx
  33:	0f b6 c0             		movzbl %al,%eax
  36:	48 f7 d8             		neg    %rax
  39:	48 09 d0             		or     %rdx,%rax
  3c:	c3                   		ret
--- conditional jump count in sat_shl64 ---
conditional_jumps=0 PASS
```

The compiler kept the construction branchless: the only
decision-making instructions are `setne` and `cmove`. (An earlier
draft masked `lost` with `0 - (kk != 0)` and the compiler turned
that mask into a conditional jump; the two-step shift
`(x >> (63 - kk)) >> 1` replaced it and the scan then passed.)
