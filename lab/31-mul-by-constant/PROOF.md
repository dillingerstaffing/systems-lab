# PROOF.md: lab/31-mul-by-constant

Environment: gcc 13.3.0 (Ubuntu 24.04.5 LTS), x86_64 host machine
(Linux 7.0.0-26-generic). `make clean` then `make`, then the three
test binaries were run in turn. Output below is the genuine build log
and the genuine run output, captured verbatim (the `exit=N` lines are
shell exit codes, not program output). All measured numbers come
from this host machine.

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_mul_const_o0 test_mul_const.c mul_const.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_mul_const_o2 test_mul_const.c mul_const.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_mul_const_asan test_mul_const.c mul_const.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
sweep: k=0..255 cases_per_k=1000000 total=256000000 mismatches=0 ns_total=5026939675 ns_per_check=19.64
checksum=18440810711898140094
throughput: values=100000000 ns_total=8441741337 ns_per_multiply=84.42 mops=11.8 sink=213905295962297104
PASS
exit=0
sweep: k=0..255 cases_per_k=1000000 total=256000000 mismatches=0 ns_total=2143926502 ns_per_check=8.37
checksum=18440810711898140094
throughput: values=100000000 ns_total=909052791 ns_per_multiply=9.09 mops=110.0 sink=213905295962297104
PASS
exit=0
sweep: k=0..255 cases_per_k=1000000 total=256000000 mismatches=0 ns_total=2408268572 ns_per_check=9.41
checksum=18440810711898140094
throughput: values=100000000 ns_total=993668784 ns_per_multiply=9.94 mops=100.6 sink=213905295962297104
PASS
exit=0
```

The first block is `-O0`, the second `-O2`, the third ASan+UBSan.

What this establishes: 256,000,000 differential checks against the
native product, 0 mismatches on every build; the FNV-1a checksum over
all outputs is identical (`18440810711898140094`) under `-O0`,
`-O2`, and the sanitizers, so the tested behavior does not depend on
optimization level or instrumentation; the ASan+UBSan run printed
no sanitizer report and exited 0. The sweep time includes the PRNG
step and the oracle multiply, so `ns_per_check` is the cost of one
full generate-and-verify case, not one `mul_const32` call.

Disassembly of `mul_const32` at `-O2` (gcc 13.3.0, x86_64), shown
here because it was actually inspected, not assumed:

```
00000000000014b0 <mul_const32>:
    14b0:	f3 0f 1e fa          	endbr64
    14b4:	40 0f b6 f6          	movzbl %sil,%esi
    14b8:	31 c0                	xor    %eax,%eax
    14ba:	31 d2                	xor    %edx,%edx
    14bc:	0f 1f 40 00          	nopl   0x0(%rax)
    14c0:	0f a3 c6             	bt     %eax,%esi
    14c3:	8d 0c 3a             	lea    (%rdx,%rdi,1),%ecx
    14c6:	0f 42 d1             	cmovb  %ecx,%edx
    14c9:	83 c0 01             	add    $0x1,%eax
    14cc:	01 ff                	add    %edi,%edi
    14ce:	83 f8 08             	cmp    $0x8,%eax
    14d1:	75 ed                	jne    14c0 <mul_const32+0x10>
    14d3:	89 d0                	mov    %edx,%eax
    14d5:	c3                   	ret
```

The compiler kept the shift-add loop as written (`bt` tests each
bit of `k`, `lea`/`cmovb` form the conditional add, `add %edi,%edi`
doubles the shift each step); it did not replace it with a hardware
`imul`. The throughput figure above (9.09 ns/multiply, 110.0 Mops/s
at `-O2`) is therefore the cost of the actual shift-add path with a
per-iteration varying `k`, which also blocks constant folding.
