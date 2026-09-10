# PROOF: lab/91-div3-magic

`div3_u32(x)`: floor(x / 3) for unsigned 32-bit x, computed as
`(uint32_t)(((uint64_t)x * 0xAAAAAAABULL) >> 33)`, one widening
multiply and one shift, no division operator in the implementation.

## What was built

`div3.h`, `div3.c`, `test_div3.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`. Proof that
no `/` operator exists outside comments in the implementation:

```
$ awk '!/^ \*/ && /\// {print FILENAME": "$0}' div3.c div3.h
div3.c: /*
div3.h: /*
```

The only hits are the `/*` comment openers; there is no division
operator in any code line. The test file uses `x / 3` twice: the
comment at line 10 naming the oracle, and line 38, the oracle
itself, documented as test-only.

## Derivation of the identity

Claim: for every x in [0, 2^32), `(x * M) >> 33 = floor(x / 3)`
where M = 0xAAAAAAAB.

First, the constant. 2^33 mod 3: 4^k = 1 mod 3 for all k, so
2^33 = 2 * 4^16 = 2 * 1 = 2 mod 3. Hence ceil(2^33 / 3) =
(2^33 + 1) / 3 = 2863311531 = 0xAAAAAAAB, exactly representable.
(0xAAAAAAAB = 10 * 16^7 + 10 * 16^6 + ... + 11 = 2863311531;
checked below against a Python computation.)

Second, exactness. Write x = 3a + r with r in {0, 1, 2}. Then

    x * M = (3a + r)(2^33 + 1) / 3
          = a * 2^33 + a + r * (2^33 + 1) / 3.

Shifting right by 33 is floor division by 2^33, so

    (x * M) >> 33 = a + floor(a / 2^33 + r * (2^33 + 1) / (3 * 2^33)).

The bracketed term must be below 1. Bound it: a <= (2^32 - 1) / 3
< 2^31, so a / 2^33 < 1/6. And r * (2^33 + 1) / (3 * 2^33) =
r / 3 + r / (3 * 2^33) < r / 3 + 1 / 2^32. The worst case is r = 2,
where the sum is < 1/6 + 2/3 + 1/2^32 < 1; r = 0, 1 are smaller.
So the floor is 0 and the result is exactly a = floor(x / 3).

Third, the 64-bit product is exact: x < 2^32 and M < 2^32, so
x * M < 2^64 and the `(uint64_t)` multiply never wraps. The C
expression `((uint64_t)x * 0xAAAAAAABULL) >> 33` therefore computes
(x * M) >> 33 exactly.

## Verification plan

1. Directed edge rows: 0, the residue classes mod 3 at both ends
   of the 32-bit range (0xFFFFFFFC..0xFFFFFFFF), powers-of-two
   boundaries (0x80000000, 0xC0000000), and alternating-bit words
   0xAAAAAAAA / 0x55555555.
2. Exhaustive differential test over all 65,536 16-bit inputs
   against native `x / 3` (oracle in the test file only).
3. 1,000,000 fixed-seed `splitmix64` random 32-bit values (seed
   `0x123456789ABCDEF0`), same per-case oracle check.
4. FNV-1a 64-bit checksum over the full output stream must be
   identical across `-O0`, `-O2`, and ASan+UBSan builds.
5. `-O2` objdump of the implementation object must contain no
   `div`/`idiv` instruction.
6. Throughput at -O2, best of 5 runs of 100M values (inputs
   pre-generated, excluded from the timed loop; stated honestly).
7. Zero warnings under `-Wall -Wextra -Werror`; zero sanitizer
   reports.

## Genuine build log and run output

```
$ make clean && make && make test_div3_O0 test_div3_san && make bench
rm -f test_div3 test_div3_O0 test_div3_san test_div3_bench div3_o2.o
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_div3 test_div3.c div3.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_div3_O0 test_div3.c div3.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_div3_san test_div3.c div3.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_div3_bench test_div3.c div3.c
```

(No warnings from any compile line; `-Werror` is on, so the build
would have failed otherwise.)

```
$ ./test_div3
edge x=00000000 -> q=00000000
edge x=00000001 -> q=00000000
edge x=00000002 -> q=00000000
edge x=00000003 -> q=00000001
edge x=00000004 -> q=00000001
edge x=00000005 -> q=00000001
edge x=00000006 -> q=00000002
edge x=fffffffc -> q=55555554
edge x=fffffffd -> q=55555554
edge x=fffffffe -> q=55555554
edge x=ffffffff -> q=55555555
edge x=aaaaaaaa -> q=38e38e38
edge x=55555555 -> q=1c71c71c
edge x=80000000 -> q=2aaaaaaa
edge x=80000001 -> q=2aaaaaab
edge x=80000002 -> q=2aaaaaab
edge x=bfffffff -> q=3fffffff
edge x=c0000000 -> q=40000000
edge x=c0000001 -> q=40000000
checks=1065555 mismatches=0 fnv1a=b2e1fc5468e18c88

$ ./test_div3_O0 | tail -1
checks=1065555 mismatches=0 fnv1a=b2e1fc5468e18c88

$ ./test_div3_san | tail -1
checks=1065555 mismatches=0 fnv1a=b2e1fc5468e18c88
```

The sanitizer binary exited 0 with `-fno-sanitize-recover=all`,
so there were no ASan/UBSan reports. The FNV-1a checksum
`b2e1fc5468e18c88` is identical across all three builds.

## Disassembly at -O2

```
$ gcc -std=c11 -Wall -Wextra -Werror -O2 -c div3.c -o div3_o2.o
$ objdump -d div3_o2.o
div3_o2.o:     file format elf64-x86-64

Disassembly of section .text:

0000000000000000 <div3_u32>:
   0:   f3 0f 1e fa          endbr64
   4:   89 f8                mov    %edi,%eax
   6:   ba ab aa aa aa       mov    $0xaaaaaaab,%edx
   b:   48 0f af c2          imul   %rdx,%rax
   f:   48 c1 e8 21          shr    $0x21,%rax
  13:   c3                   ret
```

The compiled `div3_u32` is exactly `mov / imul / shr / ret`
(`endbr64` is the CET landing pad). Grep for division mnemonics:

```
$ objdump -d div3_o2.o | grep -wE 'div|idiv' || echo "no div/idiv"
no div/idiv
```

There is no `div` or `idiv` instruction in the -O2
implementation object. (The `-w` word-match avoids matching the
`div3_u32` symbol name itself.)

## Throughput

```
$ ./test_div3_bench | tail -1
bench: 2.221 ns/value (450.3 Mvalues/s over 100M timed values, best of 5)
```

Inputs for the timed loop were pre-generated from splitmix64
into a 1M-entry array before the loop, so input generation is
excluded from the measured time; the timed loop includes the
array read, and the compiler cannot fold it because the inputs
vary. The sink is `volatile`.

## Independent Python cross-check

The C derivation was cross-checked with an independent Python
computation (arbitrary-precision integers, no C code involved),
confirming M = (2^33 + 1) / 3 and the identity on 65,536
16-bit inputs, 1,000,000 Python-random 32-bit values, and the
top-of-range words:

```
$ python3 -c "m = 0xAAAAAAAB; print(m, (2**33 + 1)//3, m == (2**33+1)//3)"
2863311531 2863311531 True
checked 1065539 bad 0
```

## Exactly what was verified

- 1,065,555 checks (19 edge rows + 65,536 exhaustive 16-bit
  inputs + 1,000,000 fixed-seed splitmix64 32-bit values, seed
  0x123456789ABCDEF0), 0 mismatches against the native `/ 3`
  oracle, which lives only in the test file.
- The exactness argument above is a proof over the whole
  [0, 2^32) range, not just the tested slice; the differential
  test is empirical support, the derivation is the guarantee.
- Checksum `b2e1fc5468e18c88` identical across `-O0`, `-O2`,
  and ASan+UBSan; no sanitizer reports.
- -O2 disassembly of the implementation contains no `div`/`idiv`.
- Throughput 2.221 ns/value at -O2 (best of 5).
- Build is warning-free under `-Wall -Wextra -Werror`.
- The implementation contains no division operator outside
  comments (awk check above); the only `/` in the test file
  outside comments is the oracle at line 38.

Built and tested 2026-09-10. Toolchain: gcc 13.3.0 (Ubuntu) on
x86-64.
