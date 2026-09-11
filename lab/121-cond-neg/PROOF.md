<!-- PROOF-HEADER
Checks: 10131084
Mismatches: 0
Checksum: 1743a170968927e0
Throughput: 4.37 ns/value at -O2, best of 5
Environment: Host
Verdict: PASS
-->
# PROOF: lab/121-cond-neg

`cond_neg(x, f) = (x ^ -f) + f` for 64-bit `x` and flag `f` in {0, 1}:
returns `x` when `f = 0` and `-x` (mod 2^64) when `f = 1`, with no
comparison and no branch on the flag.

## What was built

`cond_neg.h`, `cond_neg.c`, `test_cond_neg.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics,
no builtins, no library math in the implementation. The oracle in the
test is the plain if/else reference `f ? (0 - x) : x`, a structurally
different computation from the implementation's bit-twiddle, so
agreement pins the identity rather than a shared bug.

## The construction

From `cond_neg.c`:

```c
uint64_t cond_neg(uint64_t x, uint64_t f)
{
    return (x ^ -f) + f;
}
```

The reasoning traces to the two's-complement negation identity. In
unsigned 64-bit arithmetic, `-f` is `0` when `f = 0` and `2^64 - 1` (all
ones) when `f = 1`. XOR with all ones flips every bit, which is bitwise
NOT, and adding 1 after bitwise NOT is exactly two's-complement
negation: `~x + 1 = -x mod 2^64`. So for `f = 0` the expression is
`(x ^ 0) + 0 = x`, and for `f = 1` it is `(x ^ all-ones) + 1 = ~x + 1 = -x`.
The caller contract is only that `f` is in {0, 1}; all operations are
unsigned, so `-f`, `^`, and `+` are defined for every input.

The `-O2` disassembly of `cond_neg` (gcc 13.3.0, x86-64) is five
instructions:

```
   0:	f3 0f 1e fa         	endbr64
   4:	48 89 f0            	mov    %rsi,%rax
   7:	48 f7 d8            	neg    %rax
   a:	48 31 f8            	xor    %rdi,%rax
   d:	48 01 f0            	add    %rsi,%rax
  10:	c3                  	ret
```

The programmatic jump scan (`make disasm`, which counts objdump lines
whose mnemonic begins with `j`, the first letter of every x86 jump
mnemonic) reported `jump instructions found: 0`. `endbr64` is a branch
landing-pad marker, not a jump.

## Exactly what was verified

- 10,131,084 differential checks, 0 mismatches against the if/else
  oracle:
  - 131,072: all 65,536 16-bit `x` values x both flag values,
  - 10,000,000 fixed-seed `splitmix64` `(x, f)` pairs (seed
    `0x123456789ABCDEF0`, flag from the low bit of the odd rng output),
  - 12 edge-row checks: `x = 0`, `1`, `all-ones`, `0x8000000000000000`,
    `0xFFFF`, `0x123456789ABCDEF0` at `f = 0` and `f = 1`, printed in
    the log below.
- FNV-1a checksum `1743a170968927e0` of all results, identical across
  `-O0`, `-O2`, and ASan+UBSan; no sanitizer reports.
- Throughput 4.37 ns/value at `-O2` (best of 5 over 25M timed values).
- Build is warning-free under `-Wall -Wextra -Werror`.

## Build and run logs (actual output)

```
$ make run
./test_cond_neg
phase1 exhaustive-16bit: checks=131072, mismatches=0
phase2 random: checks=10000000, mismatches=0
phase3 edge rows (x, f, result):
  x=0000000000000000 f=0 -> 0000000000000000
  x=0000000000000000 f=1 -> 0000000000000000
  x=0000000000000001 f=0 -> 0000000000000001
  x=0000000000000001 f=1 -> ffffffffffffffff
  x=ffffffffffffffff f=0 -> ffffffffffffffff
  x=ffffffffffffffff f=1 -> 0000000000000001
  x=8000000000000000 f=0 -> 8000000000000000
  x=8000000000000000 f=1 -> 8000000000000000
  x=000000000000ffff f=0 -> 000000000000ffff
  x=000000000000ffff f=1 -> ffffffffffff0001
  x=123456789abcdef0 f=0 -> 123456789abcdef0
  x=123456789abcdef0 f=1 -> edcba98765432110
total checks: 10131084
mismatches: 0
checksum: 1743a170968927e0
PASS
./test_cond_neg_O0
phase1 exhaustive-16bit: checks=131072, mismatches=0
phase2 random: checks=10000000, mismatches=0
phase3 edge rows (x, f, result):
  x=0000000000000000 f=0 -> 0000000000000000
  x=0000000000000000 f=1 -> 0000000000000000
  x=0000000000000001 f=0 -> 0000000000000001
  x=0000000000000001 f=1 -> ffffffffffffffff
  x=ffffffffffffffff f=0 -> ffffffffffffffff
  x=ffffffffffffffff f=1 -> 0000000000000001
  x=8000000000000000 f=0 -> 8000000000000000
  x=8000000000000000 f=1 -> 8000000000000000
  x=000000000000ffff f=0 -> 000000000000ffff
  x=000000000000ffff f=1 -> ffffffffffff0001
  x=123456789abcdef0 f=0 -> 123456789abcdef0
  x=123456789abcdef0 f=1 -> edcba98765432110
total checks: 10131084
mismatches: 0
checksum: 1743a170968927e0
PASS
./test_cond_neg_san
phase1 exhaustive-16bit: checks=131072, mismatches=0
phase2 random: checks=10000000, mismatches=0
phase3 edge rows (x, f, result):
  x=0000000000000000 f=0 -> 0000000000000000
  x=0000000000000000 f=1 -> 0000000000000000
  x=0000000000000001 f=0 -> 0000000000000001
  x=0000000000000001 f=1 -> ffffffffffffffff
  x=ffffffffffffffff f=0 -> ffffffffffffffff
  x=ffffffffffffffff f=1 -> 0000000000000001
  x=8000000000000000 f=0 -> 8000000000000000
  x=8000000000000000 f=1 -> 8000000000000000
  x=000000000000ffff f=0 -> 000000000000ffff
  x=000000000000ffff f=1 -> ffffffffffff0001
  x=123456789abcdef0 f=0 -> 123456789abcdef0
  x=123456789abcdef0 f=1 -> edcba98765432110
total checks: 10131084
mismatches: 0
checksum: 1743a170968927e0
PASS
```

```
$ make disasm
gcc -std=c11 -Wall -Wextra -Werror -O2 -c cond_neg.c -o cond_neg.o
objdump -d --disassemble=cond_neg cond_neg.o
[disassembly shown above]
--- jump scan (mnemonic begins with j) ---
jump instructions found: 0
```

```
$ make bench
./test_cond_neg_bench
bench: 4.37 ns/value (228.6 Mvalues/s over 25M timed values, best of 5)
```

Built and tested 2026-09-10. Toolchain: gcc 13.3.0 (Ubuntu) on
x86-64.
