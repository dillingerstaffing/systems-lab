# PROOF: lab/97-parity-6996

`parity32(x)`: 1 if the 32-bit word `x` has an odd number of set
bits, 0 otherwise.

## What was built

`parity.h`, `parity.c`, `test_parity.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`, no
intrinsics, no builtins.

The implementation rests on two facts:

**A. The fold preserves the XOR of all bits.** Parity of a word is
the XOR of all 32 bits. Writing x as hi:lo, after `x ^= x >> 16`
the low half is `hi ^ lo`, and the XOR of its bits is
parity(hi) ^ parity(lo), which is the XOR of all 32 original bits
because XOR is associative and commutative. Repeating for k = 8 and
k = 4 puts the XOR of all 32 bits into the low nibble, so the low
nibble's parity is the word's parity.

**B. The constant 0x6996.** For every 4-bit value i, bit i of
0x6996 equals the parity of i (1 for odd popcount). Hand
derivation:

| i | binary | popcount | parity | bit i of 0x6996 |
|---|--------|----------|--------|-----------------|
| 0 | 0000 | 0 | 0 | 0 |
| 1 | 0001 | 1 | 1 | 1 |
| 2 | 0010 | 1 | 1 | 1 |
| 3 | 0011 | 2 | 0 | 0 |
| 4 | 0100 | 1 | 1 | 1 |
| 5 | 0101 | 2 | 0 | 0 |
| 6 | 0110 | 2 | 0 | 0 |
| 7 | 0111 | 3 | 1 | 1 |
| 8 | 1000 | 1 | 1 | 1 |
| 9 | 1001 | 2 | 0 | 0 |
|10 | 1010 | 2 | 0 | 0 |
|11 | 1011 | 3 | 1 | 1 |
|12 | 1100 | 2 | 0 | 0 |
|13 | 1101 | 3 | 1 | 1 |
|14 | 1110 | 3 | 1 | 1 |
|15 | 1111 | 4 | 0 | 0 |

Reading the parity column top to bottom (i = 0..15) gives
`0110100110010110` when read LSB-first, i.e. bits 15 down to 0 read
`0 1 1 0 1 0 0 1 1 0 0 1 0 1 1 0` = 0x6996 = 27030, matching the
`movl $27030` in the -O2 disassembly below. Each bit was checked
against the naive definition, not assumed.

## Verification plan

1. Differential test against a naive per-bit-loop reference:
   - all 65,536 16-bit inputs, exhaustive;
   - 1,000,000 fixed-seed splitmix64 32-bit values
     (seed 0x123456789ABCDEF0).
2. Homomorphism `parity(a^b) == parity(a)^parity(b)` on 1,000,000
   pairs from a distinct fixed seed
   (0x0FEDCBA987654321), the same check `lab/35-parity-fold` uses.
3. FNV-1a 64-bit checksum over the entire result stream must be
   identical across -O0, -O2, and ASan+UBSan builds.
4. -O2 disassembly of `parity32` must keep the fold+shift
   construction (no popcount-class instruction).
5. Throughput at -O2, best of 5 runs of 25M values.
6. Zero warnings under -Wall -Wextra -Werror; zero sanitizer
   reports.

## Genuine build log and run output

```
$ make clean && make run
rm -f test_parity test_parity_O0 test_parity_san test_parity_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_parity test_parity.c parity.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_parity_O0 test_parity.c parity.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_parity_san test_parity.c parity.c
./test_parity
checks=1065536 hom_checks=1000000 mismatches=0 fnv1a=c934e844f0908c2e
./test_parity_O0
checks=1065536 hom_checks=1000000 mismatches=0 fnv1a=c934e844f0908c2e
./test_parity_san
checks=1065536 hom_checks=1000000 mismatches=0 fnv1a=c934e844f0908c2e
```

## -O2 disassembly (fold+shift retained, no popcount)

```
parity32:
	movl	%edi, %edx
	shrl	$16, %edx
	xorl	%edi, %edx
	movl	%edx, %eax
	shrl	$8, %eax
	xorl	%edx, %eax
	movl	%eax, %ecx
	shrl	$4, %ecx
	xorl	%eax, %ecx
	movl	$27030, %eax
	andl	$15, %ecx
	shrl	%cl, %eax
	andl	$1, %eax
	ret
```

(`grep -ciE 'popcnt'` on the generated assembly: 0 matches.)

## Throughput

```
$ make bench && ./test_parity_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_parity_bench test_parity.c parity.c
checks=1065536 hom_checks=1000000 mismatches=0 fnv1a=c934e844f0908c2e
bench: 3.44 ns/value (290.4 Mvalues/s over 25M timed values, best of 5)
```

## Exactly what was verified

- 2,065,536 checks (1,065,536 differential + 1,000,000
  homomorphism), 0 mismatches against the naive reference.
- Checksum `c934e844f0908c2e` identical across -O0, -O2, and
  ASan+UBSan; no sanitizer reports.
- -O2 machine code is exactly the fold+constant-shift sequence
  above; the compiler did not substitute a popcount instruction.
- Throughput 3.44 ns/value at -O2 (best of 5).
- Build is warning-free under -Wall -Wextra -Werror.
- The 0x6996 table bits were each hand-checked against the
  per-bit definition above, and the exhaustive test confirms the
  whole module.

Built and tested 2026-09-10. Toolchain: gcc 13.3.0 (Ubuntu) on
x86-64.
