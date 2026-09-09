# PROOF.md: lab/40-sign-extend

## Build log (verbatim)

```
$ make clean && make
rm -f test_signextend test_signextend_o2 test_signextend_o0 test_signextend_asan disasm_signextend.o
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_signextend test_signextend.c
```

Build exit code: 0. Zero warnings under -Wall -Wextra -Werror.

## Run output, -O2 (verbatim)

```
$ ./test_signextend_o2
checks=4194311 mismatches=0 checksum=0x261071a94624789d
throughput: 20000000 values in 21670194 ns = 1.08 ns/value (sink=0x63119bd982b000d0)
```

Exit code: 0.

## Run output, -O0 (verbatim)

```
$ gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_signextend_o0 test_signextend.c
$ ./test_signextend_o0
checks=4194311 mismatches=0 checksum=0x261071a94624789d
throughput: 20000000 values in 65749452 ns = 3.29 ns/value (sink=0x63119bd982b000d0)
```

Build exit code: 0. Run exit code: 0.

## Run output, ASan+UBSan (verbatim)

```
$ gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_signextend_asan test_signextend.c
$ ./test_signextend_asan
checks=4194311 mismatches=0 checksum=0x261071a94624789d
throughput: 20000000 values in 29009773 ns = 1.45 ns/value (sink=0x63119bd982b000d0)
```

Build exit code: 0. Run exit code: 0. No sanitizer reports.

## What was verified

- 4,194,311 differential checks against the independent bit-test reference
  (widths 1..63 x all 65536 16-bit inputs, width 64 x all 65536 16-bit
  inputs as an identity check, plus 7 directed edge cases: w=1 with x=0,1;
  w=63 with the high bit set, clear, and all bits set; w=64 with
  INT64_MIN and all bits set). Zero mismatches in all three builds.
- FNV-1a checksum folded over every result byte is 0x261071a94624789d,
  identical across -O0, -O2, and ASan+UBSan builds.
- Throughput at -O2: 1.08 ns/value (20M values, widths 1..64 exercised,
  CLOCK_MONOTONIC timed).

## -O2 disassembly of the primitive (objdump, verbatim)

```
0000000000000000 <sign_extend_wrap>:
   0:	f3 0f 1e fa         	endbr64
   4:	b9 40 00 00 00      	mov    $0x40,%ecx
   9:	29 f1               	sub    %esi,%ecx
   b:	48 d3 e7            	shl    %cl,%rdi
   e:	48 89 f8            	mov    %rdi,%rax
  11:	48 d3 f8            	sar    %cl,%rax
  14:	c3                  	ret
```

The compiler emitted the identity directly: it computes 64 - w once
into %ecx, shifts left (shl), then shifts arithmetically right (sar) by
the same count. No extra masking, no branches.
