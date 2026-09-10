<!-- PROOF-HEADER
Checks: 10000193
Mismatches: 0
Throughput: 7.79 ns/value at -O2 over 100,000,000 values (timed loop includes PRNG step; machine variance roughly +-0.2 ns)
Environment: Host
Verdict: PASS
-->

# PROOF.md: lab/23-floor-log2

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make` from a clean
checkout, then the three test binaries were run in turn. Output below
is the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_floor_log2_o0 test_floor_log2.c floor_log2.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_floor_log2_o2 test_floor_log2.c floor_log2.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_floor_log2_asan test_floor_log2.c floor_log2.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
=== -O0 ===
boundaries+random: checked=10000193 mismatches=0 (zeros_in_random=0)
checksum=16451078516552681775
throughput: values=100000000 ns_total=1171816101 ns_per_value=11.72 sink=11
PASS
o0_exit=0
=== -O2 ===
boundaries+random: checked=10000193 mismatches=0 (zeros_in_random=0)
checksum=16451078516552681775
throughput: values=100000000 ns_total=779037917 ns_per_value=7.79 sink=11
PASS
o2_exit=0
=== asan ===
boundaries+random: checked=10000193 mismatches=0 (zeros_in_random=0)
checksum=16451078516552681775
throughput: values=100000000 ns_total=860795640 ns_per_value=8.61 sink=11
PASS
asan_exit=0
```

Timing caveats, stated honestly: the timed 100M-value loop includes
the PRNG step per value, so the ns/value figures are the cost of one
generate-and-count case, not one bare `floor_log2` call. Two extra
runs of the `-O2` binary measured 7.70 and 7.94 ns/value, so machine
variance is roughly +-0.2 ns around 7.8. The agreement of the
checksum across all three builds shows the tested behavior is
identical under `-O0`, `-O2`, and the sanitizers. Zero sanitizer
reports on the full 10,000,193-case run.

Boundary breakdown: 1 case for x = 0 (checked against the documented
`UINT64_MAX` convention, since `__builtin_clzll(0)` is undefined and
cannot serve as a reference), 64 cases of `2^k`, 64 of `2^k + 1`
(k = 0..63), 63 of `2^k - 1` (k = 1..63, i.e. 1 through `2^63 - 1`),
1 case of `UINT64_MAX`; plus 10,000,000 fixed-seed splitmix64 random
64-bit values. 193 + 10,000,000 = 10,000,193 checked, 0 mismatches.

Disassembly of `floor_log2` at `-O2` (gcc 13.3.0, x86_64), obtained
with `objdump -d test_floor_log2_o2 --disassemble=floor_log2`:

```
00000000000016a0 <floor_log2>:
    16a0:	f3 0f 1e fa          	endbr64
    16a4:	48 b9 55 55 55 55 55 	movabs $0x5555555555555555,%rcx
    16ab:	55 55 55
    16ae:	48 89 fa             	mov    %rdi,%rdx
    16b1:	48 d1 ea             	shr    $1,%rdx
    16b4:	48 09 fa             	or     %rdi,%rdx
    16b7:	48 89 d0             	mov    %rdx,%rax
    16ba:	48 c1 e8 02          	shr    $0x2,%rax
    16be:	48 09 d0             	or     %rdx,%rax
    16c1:	48 89 c2             	mov    %rax,%rdx
    16c4:	48 c1 ea 04          	shr    $0x4,%rdx
    16c8:	48 09 c2             	or     %rax,%rdx
    16cb:	48 89 d0             	mov    %rdx,%rax
    16ce:	48 c1 e8 08          	shr    $0x8,%rax
    16d2:	48 09 d0             	or     %rdx,%rax
    16d5:	48 89 c2             	mov    %rax,%rdx
    16d8:	48 c1 ea 10          	shr    $0x10,%rdx
    16dc:	48 09 c2             	or     %rax,%rdx
    16df:	48 89 d0             	mov    %rdx,%rax
    16e2:	48 c1 e8 20          	shr    $0x20,%rax
    16e6:	48 09 d0             	or     %rdx,%rax
    16e9:	48 89 c2             	mov    %rax,%rdx
    16ec:	48 d1 ea             	shr    $1,%rdx
    16ef:	48 21 ca             	and    %rcx,%rdx
    16f2:	48 b9 33 33 33 33 33 	movabs $0x3333333333333333,%rcx
    16f9:	33 33 33
    16fc:	48 29 d0             	sub    %rdx,%rax
    16ff:	48 89 c2             	mov    %rax,%rdx
    1702:	48 c1 e8 02          	shr    $0x2,%rax
    1706:	48 21 c8             	and    %rcx,%rax
    1709:	48 21 ca             	and    %rcx,%rdx
    170c:	48 01 c2             	add    %rax,%rdx
    170f:	48 89 d0             	mov    %rdx,%rax
    1712:	48 c1 e8 04          	shr    $0x4,%rax
    1716:	48 01 d0             	add    %rdx,%rax
    1719:	48 ba 0f 0f 0f 0f 0f 	movabs $0xf0f0f0f0f0f0f0f,%rdx
    1720:	0f 0f 0f
    1723:	48 21 d0             	and    %rdx,%rax
    1726:	48 89 c2             	mov    %rax,%rdx
    1729:	48 c1 ea 08          	shr    $0x8,%rdx
    172d:	48 01 d0             	add    %rdx,%rax
    1730:	48 89 c2             	mov    %rax,%rdx
    1733:	48 c1 ea 10          	shr    $0x10,%rdx
    1737:	48 01 d0             	add    %rdx,%rax
    173a:	48 89 c2             	mov    %rax,%rdx
    173d:	48 c1 ea 20          	shr    $0x20,%rdx
    1741:	48 01 d0             	add    %rdx,%rax
    1744:	83 e0 7f             	and    $0x7f,%eax
    1747:	48 83 e8 01          	sub    $0x1,%rax
    174b:	c3                   	ret
```

The compiler kept the construction exactly as written: six
`shr`/`or` propagation steps (shifts 1, 2, 4, 8, 16, 32) followed by
the SWAR popcount reduction and the final `sub $0x1`. A grep over the
disassembly for `bsr`, `lzcnt`, `tzcnt`, and `clz` found no matches:
no dedicated count instruction was emitted, and the `__builtin_clzll`
used in the test harness's reference function is never called by the
implementation. The source of the implementation itself contains no
`__builtin` of any kind.

A note on the development of the test itself: the first draft of the
boundary sweep built the candidate list with a comma expression
`(1u << 0), ((uint64_t)1 << k)` inside an array initializer and an
unconditional `2^k - 1` entry that underflowed at k = 0. That version
also never exercised x = 0 against the convention. It was replaced
with the explicit enumeration above (193 cases, each written out in
plain arithmetic), and the run output pasted in this file is from the
final harness.
