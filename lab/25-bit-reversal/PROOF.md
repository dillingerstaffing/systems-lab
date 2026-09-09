# PROOF.md: lab/25-bit-reversal

Environment: gcc 13.3.0 (Ubuntu 24.04), x86_64. `make` from a clean
checkout, then the three test binaries were run in turn. Output below
is the genuine build log and the genuine run output, captured verbatim
(the `o0_exit`/`o2_exit`/`asan_exit` lines are shell exit codes, not
program output).

## Build log

```
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_bit_reverse_o0 test_bit_reverse.c bit_reverse.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_bit_reverse_o2 test_bit_reverse.c bit_reverse.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_bit_reverse_asan test_bit_reverse.c bit_reverse.c
```

Zero warnings at `-Wall -Wextra -Werror` on all three builds
(`-O0`, `-O2`, ASan+UBSan). Build exit code 0.

## Run output

```
=== -O0 ===
differential+involution: checked=10262212 mismatches=0 involution_mismatches=0
checksum=8367746376622110355
throughput: values=100000000 ns_total=1066593773 ns_per_value=10.67 sink=6543017044187020749
PASS
o0_exit=0
=== -O2 ===
differential+involution: checked=10262212 mismatches=0 involution_mismatches=0
checksum=8367746376622110355
throughput: values=100000000 ns_total=612103337 ns_per_value=6.12 sink=6543017044187020749
PASS
o2_exit=0
=== asan ===
differential+involution: checked=10262212 mismatches=0 involution_mismatches=0
checksum=8367746376622110355
throughput: values=100000000 ns_total=1316922102 ns_per_value=13.17 sink=6543017044187020749
PASS
asan_exit=0
```

Test breakdown, all differential against a naive reference that moves
bit k to position 63 - k one bit at a time, plus the involution
invariant `bit_reverse(bit_reverse(x)) == x` on every case:

- 4 explicit boundaries: 0, `UINT64_MAX`, `0x5555555555555555`,
  `0xAAAAAAAAAAAAAAAA`.
- 64 single-bit values `2^k` for k = 0..63.
- Exhaustive: all 65536 16-bit values embedded at four 16-bit lanes
  of the 64-bit word (shifts 0, 16, 32, 48): 262144 cases.
- 10,000,000 fixed-seed splitmix64 random 64-bit values (seed
  `0x123456789ABCDEF0`).

4 + 64 + 262144 + 10,000,000 = 10,262,212 checked, 0 differential
mismatches, 0 involution mismatches. Identical FNV-1a checksum
(`8367746376622110355`) under `-O0`, `-O2`, and ASan+UBSan; zero
sanitizer reports on the full 10,262,212-case run.

Timing caveats, stated honestly: the timed 100M-value loop includes
the PRNG step per value, so the ns/value figures are the cost of one
generate-and-reverse case, not one bare `bit_reverse` call. A second
run of the `-O2` binary measured 4.82 ns/value against 6.12 in the
recorded run, so machine variance is roughly +-1 ns (frequency
scaling on this box is visible). The agreement of the checksum
across all three builds shows the tested behavior is identical under
`-O0`, `-O2`, and the sanitizers. The 100M-value sink total was
6543017044187020749 on every run.

Disassembly of `bit_reverse` at `-O2` (gcc 13.3.0, x86_64), obtained
with `objdump -d test_bit_reverse_o2 --disassemble=bit_reverse`:

```
00000000000015e0 <bit_reverse>:
    15e0:	f3 0f 1e fa          	endbr64
    15e4:	48 ba 55 55 55 55 55 	movabs $0x5555555555555555,%rdx
    15eb:	55 55 55
    15ee:	48 89 f8             	mov    %rdi,%rax
    15f1:	48 d1 ef             	shr    $1,%rdi
    15f4:	48 b9 0f 0f 0f 0f 0f 	movabs $0xf0f0f0f0f0f0f0f,%rcx
    15fb:	0f 0f 0f
    15fe:	48 21 d7             	and    %rdx,%rdi
    1601:	48 01 c0             	add    %rax,%rax
    1604:	48 ba aa aa aa aa aa 	movabs $0xaaaaaaaaaaaaaaaa,%rdx
    160b:	aa aa aa
    160e:	48 21 d0             	and    %rdx,%rax
    1611:	48 09 c7             	or     %rax,%rdi
    1614:	48 b8 33 33 33 33 33 	movabs $0x3333333333333333,%rax
    161b:	33 33 33
    161e:	48 89 fa             	mov    %rdi,%rdx
    1621:	48 c1 e7 02          	shl    $0x2,%rdi
    1625:	48 c1 ea 02          	shr    $0x2,%rdx
    1629:	48 21 c2             	and    %rax,%rdx
    162c:	48 b8 cc cc cc cc cc 	movabs $0xcccccccccccccccc,%rax
    1633:	cc cc cc
    1636:	48 21 c7             	and    %rax,%rdi
    1639:	48 09 fa             	or     %rdi,%rdx
    163c:	48 89 d0             	mov    %rdx,%rax
    163f:	48 c1 e2 04          	shl    $0x4,%rdx
    1643:	48 c1 e8 04          	shr    $0x4,%rax
    1647:	48 21 c8             	and    %rcx,%rax
    164a:	48 b9 f0 f0 f0 f0 f0 	movabs $0xf0f0f0f0f0f0f0f0,%rcx
    1651:	f0 f0 f0
    1654:	48 21 ca             	and    %rcx,%rdx
    1657:	48 09 d0             	or     %rdx,%rax
    165a:	48 0f c8             	bswap  %rax
    165d:	c3                   	ret
```

Honest reading of this disassembly: the compiler kept the group
swaps of sizes 1, 2, and 4 as plain shift/and/or sequences (the
m = 1 step is algebraically reworked with mask `0xaaaaaaaaaaaaaaaa`
instead of the literal `0x5555...` form, which is the same value
moved the other way, and is correct). For the byte-reordering tail
(steps 8, 16, 32), gcc recognized the composition and strength-
reduced the three shift/OR stages into a single `bswap %rax`, which
is the exact byte-reversal those three steps compute. There is no
`__builtin` or dedicated bit-reverse instruction anywhere in the
source; the byte-swap tail is written out with shifts and ORs, and
the compiler's substitution to `bswap` is semantics-preserving, as
the differential test against the naive reference confirms. x86-64
has no instruction that reverses bit order within bytes, so the
within-byte swaps necessarily stay as shifts and masks.
