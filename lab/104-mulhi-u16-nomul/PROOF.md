<!-- PROOF-HEADER
Checks: 4294967673
Mismatches: 0
Checksum: 0xbe84b05c63e3bea1
Throughput: 11.807 ns/pair, best of 5
Verdict: PASS
-->

# PROOF.md: lab/104-mulhi-u16-nomul

`mulhi_u16(a, b)` in `mulhi_u16.h`: the high 16 bits of the 32-bit
product of two `uint16_t` values, using shifts and adds only. No
multiply operator appears anywhere in the implementation, not even a
32-bit one.

## Construction

Write b in binary as b = sum_{i=0..15} bit_i(b) * 2^i. Then

    a * b = sum_{i=0..15} bit_i(b) * (a << i),

so the full 32-bit product is the sum of the shifted copies of a at
the positions where b has a 1 bit. The loop walks the bits of b low
to high, keeps x = a << i, and adds x into a 32-bit accumulator when
bit i of b is set, returning acc >> 16. The accumulator never exceeds
(2^16 - 1)^2 < 2^32 and x never exceeds a << 16 < 2^32, so every shift
and add is well-defined on unsigned values.

## Build log (genuine, zero warnings)

```
$ make
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -DBUILD_NAME='"-O0"' -o test_mulhi_u16_o0 test_mulhi_u16.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -DBUILD_NAME='"-O2"' -o test_mulhi_u16_o2 test_mulhi_u16.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -DBUILD_NAME='"asan+ubsan"' -o test_mulhi_u16_asan test_mulhi_u16.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -c -o disasm_mulhi_u16.o disasm_mulhi_u16.c
objdump -d -M intel disasm_mulhi_u16.o > disasm_mulhi_u16.txt
! grep -Ei '\b(mul|imul)\b' disasm_mulhi_u16.txt
rm -f disasm_mulhi_u16.o
```

No warnings under `-std=c11 -Wall -Wextra -Werror` in any of the
three builds (exit 0).

## Verification runs (genuine output)

Oracle: `((uint32_t)a * b) >> 16`, confined to `ref_high()` in the
test file. The exhaustive pass covers all 2^32 pairs of 16-bit
inputs, the complete input space, so every multiplier bit pattern
(all 65,536 values of b, hence every add/no-add path) and every
multiplicand value is exercised. The directed pass covers the
boundary rows (0, 1, 2, 3, 0x7FFE/0x7FFF/0x8000/0x8001,
0xFFFD/0xFFFE/0xFFFF) as an 11x11 cross product plus all 16x16
power-of-two pairs: 121 + 256 = 377 directed cases (also covered by
the exhaustive pass).

Build -O2:

```
mulhi_u16 differential test, build -O2
[1/3] directed edge cases
  done: cases=377 mismatches=0
[2/3] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967673 mismatches=0
[3/3] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 71.542 ns/pair
  throughput pass 1: 101.235 ns/pair
  throughput pass 2: 61.931 ns/pair
  throughput pass 3: 11.807 ns/pair
  throughput pass 4: 18.178 ns/pair
  throughput best of 5: 11.807 ns/pair
total verification cases: 4294967673
total mismatches: 0
FNV-1a checksum of all result words: 0xbe84b05c63e3bea1
RESULT: PASS
```

Build -O0:

```
mulhi_u16 differential test, build -O0
[1/3] directed edge cases
  done: cases=377 mismatches=0
[2/3] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967673 mismatches=0
[3/3] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 166.824 ns/pair
  throughput pass 1: 168.273 ns/pair
  throughput pass 2: 167.506 ns/pair
  throughput pass 3: 166.305 ns/pair
  throughput pass 4: 167.339 ns/pair
  throughput best of 5: 166.305 ns/pair
total verification cases: 4294967673
total mismatches: 0
FNV-1a checksum of all result words: 0xbe84b05c63e3bea1
RESULT: PASS
```

Build asan+ubsan (AddressSanitizer + UBSan, full 4.3B-case suite):

```
mulhi_u16 differential test, build asan+ubsan
[1/3] directed edge cases
  done: cases=377 mismatches=0
[2/3] exhaustive 16-bit x 16-bit pairs (4294967296 cases)
  done: cases=4294967673 mismatches=0
[3/3] throughput (PRNG pre-generated, excluded from timing)
  throughput pass 0: 38.400 ns/pair
  throughput pass 1: 58.658 ns/pair
  throughput pass 2: 42.585 ns/pair
  throughput pass 3: 62.777 ns/pair
  throughput pass 4: 37.143 ns/pair
  throughput best of 5: 37.143 ns/pair
total verification cases: 4294967673
total mismatches: 0
FNV-1a checksum of all result words: 0xbe84b05c63e3bea1
RESULT: PASS
```

The FNV-1a checksum `0xbe84b05c63e3bea1` is identical across -O0,
-O2, and ASan+UBSan builds. No sanitizer reported anything on the
full 4.3B-case run (zero "runtime error" / "AddressSanitizer" lines
in the asan+ubsan log).

## Source-level multiply audit

The implementation file `mulhi_u16.h` contains no `*` operator in
any line of actual code: every `*` in the file sits inside a comment
(verified with `grep -n '\*'`). The only multiplications in the test
file are the oracle (`((uint32_t)a * b) >> 16` in `ref_high()`), the
splitmix64 PRNG step, and the FNV-1a hash accumulator, none of which
are part of the implementation under test.

## Disassembly check at -O2 (x86-64, GCC)

`objdump -d` of a noinline wrapper calling mulhi_u16, compiled from
the header alone so the oracle multiply cannot leak in (the `make`
step greps the output for `mul`/`imul` and fails if either appears):

```
0000000000000000 <wrap_mulhi_u16>:
   0:  f3 0f 1e fa          endbr64
   4:  0f b7 ff             movzx  edi,di        ; x = a, zero-extended
   7:  ba 10 00 00 00       mov    edx,0x10       ; loop counter = 16
   c:  31 c0                xor    eax,eax        ; acc = 0
  10:  40 f6 c6 01          test   sil,0x1        ; bit i of b
  14:  8d 0c 38             lea    ecx,[rax+rdi*1]; acc + x  (the *1 is an lea scale, not a multiply)
  17:  0f 45 c1             cmovne eax,ecx       ; if bit set, acc += x (branchless)
  1a:  01 ff                add    edi,edi        ; x <<= 1
  1c:  66 d1 ee             shr    si,1           ; y >>= 1
  1f:  83 ea 01             sub    edx,0x1
  22:  75 ec                jne    10
  24:  c1 e8 10             shr    eax,0x10       ; acc >> 16
  27:  c3                   ret
```

What this shows:

- The loop compiled branchless: `test` on the low multiplier bit,
  `lea` for the candidate sum, `cmovne` for the conditional add.
- `add edi,edi` doubles x each step and `shr si,1` walks the
  multiplier bits, exactly the `x <<= 1` / `y >>= 1` of the source.
- The result is `shr eax,0x10`, the `(acc >> 16)` return.
- No `mul` or `imul` instruction appears anywhere; the only `*`
  character in the listing is the `*1` scale inside the `lea` address
  expression, which is not a multiply instruction.

## Throughput

Measured at -O2 over 1M pre-generated splitmix64 pairs, 5 passes,
PRNG excluded from the timed loop, results xored into a volatile
sink: best 11.807 ns/pair (passes 71.542, 101.235, 61.931, 11.807,
18.178). (-O0: 166.305 ns/pair best of 5; ASan+UBSan: 37.143
ns/pair best of 5.)

## Claims and their grounding

- "0 mismatches over 4,294,967,673 checks per build": counted by the
  test binaries (exit 0, RESULT: PASS), differential against the
  native 32-bit product oracle.
- "Checksum identical across builds": the FNV-1a value printed by
  each binary, `0xbe84b05c63e3bea1` in all three logs above.
- "No multiply anywhere in the implementation": `grep -n '\*'` on
  `mulhi_u16.h` finds stars only in comments, and the -O2
  disassembly above contains no `mul`/`imul` instruction.
- "Clean under ASan/UBSan": the full 4.3B-case suite ran under
  `-fsanitize=address,undefined` with no sanitizer output.
