# PROOF: lab/80-bcd-digit-valid

`bcd_invalid_mask(w)`: given a 16-bit word w holding four packed BCD
nibbles (nibble 0 = bits 0..3, least significant), returns a 4-bit mask
where bit i is 1 iff nibble i holds a value in 10..15, i.e. is not a
valid BCD digit.

## What was built

`bcd.h`, `bcd.c`, `test_bcd.c`, `Makefile`, `README.md`, this file.
Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no
builtins.

The construction rests on three facts, each proved below.

### Fact 1: the add-guard identity

For a nibble value n in 0..15: n + 6 >= 16 iff n >= 10. The forward
direction: n >= 10 gives n + 6 >= 16 directly. The reverse: if
n <= 9 then n + 6 <= 15 < 16. So adding 6 to a nibble carries out of
the nibble exactly when the nibble is invalid (10..15), provided the
carry into the nibble is 0. (If the carry in were 1, a valid nibble 9
would give 9 + 6 + 1 = 16 and falsely carry out. That is exactly the
aliasing hazard Fact 2 defeats.)

### Fact 2: lane isolation (no inter-nibble carry aliasing)

The two adds are

```
t_even = (w & 0x0F0F) + 0x0606   (lanes: nibbles 0 and 2)
t_odd  = (w & 0xF0F0) + 0x6060   (lanes: nibbles 1 and 3)
```

Hand-verified bit layout of the constants (6 = 0b0110):

- 0x0F0F: bits 0..3 and 8..11 are 1 (nibbles 0, 2 active), all other
  bits 0. 0x0606: bits 1, 2 and 9, 10 are 1, all other bits 0.
- 0xF0F0: bits 4..7 and 12..15 are 1 (nibbles 1, 3 active), all other
  bits 0. 0x6060: bits 5, 6 and 13, 14 are 1, all other bits 0.

In each add, the two active lanes sit 8 bits apart, and every bit in
the gap between them is 0 in BOTH addends: even add gaps are bits
4..7 (a bits 0 since 0x0F0F has nibble 1 zeroed; b bits 0 since 0x0606
bit 4..7 = 0); odd add gaps are bits 0..3 and 8..11 (a bits 0 by the
0xF0F0 mask; b bits 0 since 0x6060 bits 0..3 and 8..11 = 0).

A binary full adder with both addend bits 0 outputs carry 0 regardless
of carry in: c_out = (a&b) | (c_in & (a|b)) = 0 | (c_in & 0) = 0. So:

- Even add: bits 4..7 have a_k = b_k = 0, hence c_5 = 0, and by
  induction c_6 = c_7 = c_8 = 0. The carry into nibble 2's lane
  (bit 8) is 0, and c_0 = 0 into nibble 0's lane. A carry generated at
  nibble 0 (c_4) dies at bit 4 (c_5 = 0); it can never reach bit 8.
- Odd add: bits 0..3 have a_k = b_k = 0, hence c_1..c_4 = 0 (carry 0
  into nibble 1's lane at bit 4). Bits 8..11 have a_k = b_k = 0, hence
  c_9..c_12 = 0 (carry 0 into nibble 3's lane at bit 12). A carry
  generated at nibble 1 (c_8) dies at bit 8 (c_9 = 0); it can never
  reach bit 12.

Therefore the carry into every lane is provably 0, and no generated
carry can ever propagate from one nibble's lane into another's. By
Fact 1, each lane's carry out happens exactly for an invalid nibble.

Contrast the naive form `w + 0x6666` used only as a negative control
in the test: there a carry generated at a low invalid nibble enters
the next nibble's lane as carry in = 1, so a valid 9 above an invalid
nibble gives 9 + 6 + 1 = 16 and falsely carries out. The test measures
this: the naive form misclassifies 4,572 of the 65,536 inputs
(`naive_form_wrong=4572` in the log below), while the shipped
construction matches the reference on all of them.

### Fact 3: the carry extraction identity

For t = a + b, bit k of the sum is t_k = a_k ^ b_k ^ c_k where c_k is
the carry into bit k, so c_k = t_k ^ a_k ^ b_k. The carry out of nibble
i (bits 4i..4i+3) is the carry into bit 4i+4. At each lane boundary the
two addend bits are 0, verified from the constants above:

- nibble 0: c_4 = t_4 ^ a_4 ^ b_4; a_4 = 0 (0x0F0F), b_4 = 0 (0x0606).
  So c_4 = bit 4 of t_even.
- nibble 1: c_8 = t_8 ^ a_8 ^ b_8; a_8 = 0 (0xF0F0 has nibble 2
  zeroed), b_8 = 0 (0x6060). So c_8 = bit 8 of t_odd.
- nibble 2: c_12 = t_12 ^ a_12 ^ b_12; a_12 = 0 (0x0F0F has nibble 3
  zeroed), b_12 = 0 (0x0606). So c_12 = bit 12 of t_even.
- nibble 3: c_16 = t_16 ^ a_16 ^ b_16; a_16 = b_16 = 0 (both addends
  fit in 16 bits; the add is done in uint32_t). So c_16 = bit 16 of
  t_odd.

Each mask bit is therefore a single sum bit:

```
m = ((t_even >> 4)  & 1) << 0
  | ((t_odd  >> 8)  & 1) << 1
  | ((t_even >> 12) & 1) << 2
  | ((t_odd  >> 16) & 1) << 3
```

Together: mask bit i = carry out of lane i = 1 iff nibble i >= 10, by
Facts 1 and 2, with no aliasing possible by the full-adder argument.

## Verification plan

1. Differential test against an independent per-nibble loop reference
   over ALL 65,536 16-bit inputs; 0 mismatches required.
2. 18 directed rows chosen to alias under the naive form
   (0x9A00, 0x0A00, 0x9A9A, 0xF900, 0x0900, 0x000A, 0xA000, ...),
   each checked against the reference, with the naive form's answer
   printed alongside to show the hazard is real and defeated.
3. FNV-1a 64-bit checksum over the entire 65,536-output stream must be
   identical across -O0, -O2, and ASan+UBSan builds.
4. -O2 disassembly of `bcd_invalid_mask` must keep the masked
   add/extract construction.
5. Throughput at -O2, best of 5 (1M pre-filled values, 25 passes, RNG
   cost excluded by prefilling).
6. Zero warnings under -Wall -Wextra -Werror; zero sanitizer reports.

## Genuine build log and run output

```
$ make clean && make run
rm -f test_bcd test_bcd_O0 test_bcd_san test_bcd_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_bcd test_bcd.c bcd.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_bcd_O0 test_bcd.c bcd.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_bcd_san test_bcd.c bcd.c
./test_bcd
directed w=9a00 mask=4 ref=4 naive=c
directed w=0a00 mask=4 ref=4 naive=4
directed w=9a9a mask=5 ref=5 naive=f
directed w=f900 mask=8 ref=8 naive=8
directed w=0900 mask=0 ref=0 naive=0
directed w=000a mask=1 ref=1 naive=1
directed w=a000 mask=8 ref=8 naive=8
directed w=00a0 mask=2 ref=2 naive=2
directed w=0a90 mask=4 ref=4 naive=4
directed w=ffff mask=f ref=f naive=f
directed w=0000 mask=0 ref=0 naive=0
directed w=9999 mask=0 ref=0 naive=0
directed w=aaaa mask=f ref=f naive=f
directed w=f000 mask=8 ref=8 naive=8
directed w=000f mask=1 ref=1 naive=1
directed w=9909 mask=0 ref=0 naive=0
directed w=0990 mask=0 ref=0 naive=0
directed w=a9a9 mask=a ref=a naive=e
checks=65554 mismatches=0 naive_form_wrong=4572 fnv1a=ffd4445043425186
./test_bcd_O0
directed w=9a00 mask=4 ref=4 naive=c
directed w=0a00 mask=4 ref=4 naive=4
directed w=9a9a mask=5 ref=5 naive=f
directed w=f900 mask=8 ref=8 naive=8
directed w=0900 mask=0 ref=0 naive=0
directed w=000a mask=1 ref=1 naive=1
directed w=a000 mask=8 ref=8 naive=8
directed w=00a0 mask=2 ref=2 naive=2
directed w=0a90 mask=4 ref=4 naive=4
directed w=ffff mask=f ref=f naive=f
directed w=0000 mask=0 ref=0 naive=0
directed w=9999 mask=0 ref=0 naive=0
directed w=aaaa mask=f ref=f naive=f
directed w=f000 mask=8 ref=8 naive=8
directed w=000f mask=1 ref=1 naive=1
directed w=9909 mask=0 ref=0 naive=0
directed w=0990 mask=0 ref=0 naive=0
directed w=a9a9 mask=a ref=a naive=e
checks=65554 mismatches=0 naive_form_wrong=4572 fnv1a=ffd4445043425186
./test_bcd_san
directed w=9a00 mask=4 ref=4 naive=c
directed w=0a00 mask=4 ref=4 naive=4
directed w=9a9a mask=5 ref=5 naive=f
directed w=f900 mask=8 ref=8 naive=8
directed w=0900 mask=0 ref=0 naive=0
directed w=000a mask=1 ref=1 naive=1
directed w=a000 mask=8 ref=8 naive=8
directed w=00a0 mask=2 ref=2 naive=2
directed w=0a90 mask=4 ref=4 naive=4
directed w=ffff mask=f ref=f naive=f
directed w=0000 mask=0 ref=0 naive=0
directed w=9999 mask=0 ref=0 naive=0
directed w=aaaa mask=f ref=f naive=f
directed w=f000 mask=8 ref=8 naive=8
directed w=000f mask=1 ref=1 naive=1
directed w=9909 mask=0 ref=0 naive=0
directed w=0990 mask=0 ref=0 naive=0
directed w=a9a9 mask=a ref=a naive=e
checks=65554 mismatches=0 naive_form_wrong=4572 fnv1a=ffd4445043425186
```

Note on the directed rows: mask values read nibble 0 = least
significant (e.g. 0x9A00 has the invalid nibble A at bits 8..11, so
mask=4; the reference agrees). The `naive=` column shows the naive
w + 0x6666 form misclassifying exactly the rows where a carry chain
crosses a valid nibble (0x9A00 -> c, 0x9A9A -> f, 0xA9A9 -> e), while
the shipped mask matches the reference on every row.

## -O2 disassembly (masked-add construction retained)

```
bcd_invalid_mask:
	movl	%edi, %eax
	andl	$61680, %edi      # w & 0xF0F0
	andl	$3855, %eax       # w & 0x0F0F
	addl	$24672, %edi      # + 0x6060  -> t_odd
	leal	1542(%rax), %edx  # + 0x0606  -> t_even
	movl	%edx, %eax
	shrl	$12, %edx         # bit 12 of t_even (nibble 2 carry out)
	shrl	$4, %eax          # bit 4 of t_even  (nibble 0 carry out)
	sall	$2, %edx
	andl	$1, %eax
	orl	%edx, %eax
	movl	%edi, %edx
	shrl	$8, %edi          # bit 8 of t_odd (nibble 1 carry out)
	shrl	$16, %edx         # bit 16 of t_odd (nibble 3 carry out)
	addl	%edi, %edi
	sall	$3, %edx
	andl	$2, %edi
	orl	%edx, %eax
	orl	%edi, %eax          # the four bits become the 4-bit mask
	ret
```

(61680 = 0xF0F0, 3855 = 0x0F0F, 24672 = 0x6060, 1542 = 0x0606.
`grep -ciE 'popcnt'` on the generated assembly: 0 matches.)

## Throughput

```
$ make bench && ./test_bcd_bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_bcd_bench test_bcd.c bcd.c
checks=65554 mismatches=0 naive_form_wrong=4572 fnv1a=ffd4445043425186
bench: 4.27 ns/value (234.2 Mvalues/s over 26214400 timed values, best of 5)
```

Methodology: 1M splitmix64 values pre-filled into an array, then 25
passes over the array timed with CLOCK_MONOTONIC; the reported number
is the best of 5 such runs, so the RNG cost is outside the timed
region and only the mask computation is measured.

## Exactly what was verified

- 65,554 checks (65,536 exhaustive differential + 18 directed rows),
  0 mismatches against the independent per-nibble reference.
- The naive w + 0x6666 form misclassifies 4,572 of the 65,536 inputs,
  confirming the carry-aliasing hazard is real; the shipped
  construction agrees with the reference on all of them.
- Checksum `ffd4445043425186` identical across -O0, -O2, and
  ASan+UBSan; no sanitizer reports.
- -O2 machine code is exactly the two masked adds plus the four
  bit-extractions; the compiler did not substitute anything.
- Throughput 4.27 ns/value at -O2 (best of 5).
- Build is warning-free under -Wall -Wextra -Werror.
- The add-guard identity, the lane-isolation bit positions, and the
  carry extraction were each derived by hand above and confirmed by
  the exhaustive test.

Built and tested 2026-09-10. Toolchain: gcc 13.3.0 (Ubuntu) on
x86-64.
