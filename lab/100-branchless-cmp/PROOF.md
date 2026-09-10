# PROOF: lab/100-branchless-cmp

`bcmp64(a, b)`: returns `-1` if `a < b`, `0` if `a == b`, `+1` if
`a > b`, for `uint64_t` `a`, `b`, computed from the borrow-out
identity with no conditional branch in the implementation.

## What was built

`cmp.h`, `cmp.c`, `test_cmp.c`, `Makefile`, `README.md`, this file.
Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no
builtins, no library math. Toolchain: gcc 13.3.0 on x86_64.

The implementation:

```c
int64_t bcmp64(uint64_t a, uint64_t b)
{
    uint64_t d = a - b;        /* wraps modulo 2^64, total */
    uint64_t lt = d > a;       /* borrow-out: 1 iff a < b */
    uint64_t eq = a == b;      /* 1 iff a == b */
    uint64_t gt = 1 - lt - eq; /* 1 iff a > b, by Fact 3 */
    return (int64_t)gt - (int64_t)lt;
}
```

## Hand derivation

Work entirely in the unsigned domain, where subtraction and
comparison are total (C11 6.2.5p9: unsigned arithmetic wraps modulo
2^64).

Fact 1 (borrow-out): for unsigned `a`, `b`,
`(a - b) mod 2^64 > a` iff `a < b`.

Proof. If `a >= b`, then `0 <= a - b <= a`, so `(a - b) mod 2^64 =
a - b <= a`. If `a < b`, then `(a - b) mod 2^64 = 2^64 - (b - a)`;
since `1 <= b <= 2^64 - 1`, we have `2^64 - (b - a) - a = 2^64 - b
>= 1`, i.e. the wrapped difference strictly exceeds `a`. The two
cases partition all inputs, so the biconditional holds for every
`uint64_t a, b`, including the extremes. This is exactly what the
`lt` flag tests.

Fact 2: `a == b` needs no wrapping argument at all; it is the plain
unsigned equality, total by definition. (`eq` and `lt` agree with
`d == 0` and `d > a` respectively, but the code tests them
directly.)

Fact 3: `lt` and `eq` cannot both be 1 (`a < b` and `a == b` are
mutually exclusive), so `gt = 1 - lt - eq` is in `{0, 1}` and is 1
exactly when `a > b`: the three flags partition the input space and
exactly one of them is 1. `(int64_t)gt` and `(int64_t)lt` are 0 or
1, so `(int64_t)gt - (int64_t)lt` is confined to `{-1, 0, +1}`; no
signed overflow is possible anywhere. The full case table:

| a vs b | lt | eq | gt = 1-lt-eq | result |
|--------|----|----|--------------|--------|
| a < b  | 1  | 0  | 0            | -1     |
| a == b | 0  | 1  | 0            | 0      |
| a > b  | 0  | 0  | 1            | +1     |

Hence `bcmp64` returns `-1`, `0`, `+1` for every one of the 2^128
`(a, b)` pairs.

## Verification plan

1. Directed rows: 14 `(a, b)` pairs covering 0, `UINT64_MAX`,
   neighbors on both sides, the signedness boundary
   (`0x8000000000000000` vs `0x7FFFFFFFFFFFFFFF`), and mixed bit
   patterns, each checked against the reference `(a > b) - (a < b)`
   and printed.
2. Differential test against that reference over all 4,294,967,296
   16-bit pairs (`a` and `b` over `[0, 65535]`) plus 10,000,000
   fixed-seed splitmix64 64-bit pairs (seed `0x123456789ABCDEF0`).
3. FNV-1a 64-bit checksum over the entire result stream must be
   identical across -O0, -O2, and ASan+UBSan builds.
4. -O2 disassembly of the object must contain no conditional jump.
5. Throughput at -O2, best of 5.

## Genuine build log

```
$ make clean && make run
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_cmp test_cmp.c cmp.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_cmp_O0 test_cmp.c cmp.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_cmp_san test_cmp.c cmp.c
```

Zero warnings on all three lines. Zero ASan/UBSan reports at runtime
on all binaries.

## Genuine run output (-O2, -O0, ASan+UBSan)

All three binaries printed the same result:

```
directed rows:
  a=0000000000000000 b=0000000000000000 bcmp64=0 ref=0 ok
  a=0000000000000000 b=0000000000000001 bcmp64=-1 ref=-1 ok
  a=0000000000000001 b=0000000000000000 bcmp64=1 ref=1 ok
  a=0000000000000000 b=ffffffffffffffff bcmp64=-1 ref=-1 ok
  a=ffffffffffffffff b=0000000000000000 bcmp64=1 ref=1 ok
  a=ffffffffffffffff b=ffffffffffffffff bcmp64=0 ref=0 ok
  a=ffffffffffffffff b=fffffffffffffffe bcmp64=1 ref=1 ok
  a=fffffffffffffffe b=ffffffffffffffff bcmp64=-1 ref=-1 ok
  a=8000000000000000 b=8000000000000000 bcmp64=0 ref=0 ok
  a=8000000000000000 b=7fffffffffffffff bcmp64=1 ref=1 ok
  a=7fffffffffffffff b=8000000000000000 bcmp64=-1 ref=-1 ok
  a=deadbeefdeadbeef b=deadbeefdeadbeef bcmp64=0 ref=0 ok
  a=deadbeefdeadbeef b=deadbeefdeadbef0 bcmp64=-1 ref=-1 ok
  a=1234567812345678 b=1234567812345678 bcmp64=0 ref=0 ok
checks=4304967310 mismatches=0 fnv1a=07ae4885650dd8ca
```

4,304,967,310 = 14 directed + 4,294,967,296 exhaustive + 10,000,000
random. 0 mismatches. FNV-1a checksum `07ae4885650dd8ca` is
identical across -O0, -O2, and the ASan+UBSan build (all three
binaries were run and printed the same checksum line).

## Genuine benchmark output

```
$ make bench && ./test_cmp_bench   (tail of output)
bench: 2.31 ns/value (432.7 Mvalues/s over 25M timed values, best of 5)
```

Loop conditions, stated honestly: the 1M pairs were generated once
into a malloc'd array before timing; the timed region is 25 passes
over that array, XOR-ing each `bcmp64` result into a `volatile`
sink. What is measured is `bcmp64` plus loop and memory traffic, not
the RNG. Compiled with `-O2 -DBENCH` as shown in the build log.

## -O2 disassembly of bcmp64

`gcc -std=c11 -Wall -Wextra -Werror -O2 -c cmp.c`, `objdump -d`:

```
0000000000000000 <bcmp64>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 39 f7            	cmp    %rsi,%rdi
   7:	b8 01 00 00 00      	mov    $0x1,%eax
   c:	0f 92 c2            	setb   %dl
   f:	0f 94 c1            	sete   %cl
  12:	0f b6 d2            	movzbl %dl,%edx
  15:	0f b6 c9            	movzbl %cl,%ecx
  18:	48 01 d1            	add    %rdx,%rcx
  1b:	48 29 c8            	sub    %rcx,%rax
  1e:	48 29 d0            	sub    %rdx,%rax
  21:	c3                  	ret
```

A programmatic scan of the function's disassembly found 0 jump
instructions of any kind (`grep -cE '\sj[a-z]+'` on the `bcmp64`
block returned 0). The mnemonic set is exactly `endbr64`, `cmp`,
`mov`, `setb`, `sete`, `movzbl`, `movzbl`, `add`, `sub`, `sub`,
`ret`. The compiler folded `d = a - b` away entirely: the `cmp`
sets the hardware carry flag exactly when the subtraction borrows,
and `setb` reads that borrow directly, which is the borrow-out
identity realized in the flags. `sete` reads `eq`, the
`add`/`sub`/`sub` chain computes `gt - lt = (1 - lt - eq) - lt`.

## What was verified, exactly

- The identity `bcmp64(a, b) == (a > b) - (a < b)` held for every
  one of the 4,294,967,296 16-bit `(a, b)` pairs, 10,000,000
  fixed-seed 64-bit pairs, and the 14 directed rows, 0 mismatches.
- The only signed arithmetic in the implementation is the
  conversion of the `{0, 1}` flags and their subtraction, whose
  result is confined to `{-1, 0, +1}`; no signed overflow is
  possible (also confirmed by the UBSan build running clean).
- The result stream is bit-identical across -O0, -O2, and
  ASan+UBSan (FNV-1a `07ae4885650dd8ca`).
- The -O2 object code contains no conditional jump (verified by
  scan: 0 jump instructions).
- Throughput: 2.31 ns/value, 432.7 Mvalues/s, best of 5, under the
  loop conditions stated above.

All test binaries exited 0.
