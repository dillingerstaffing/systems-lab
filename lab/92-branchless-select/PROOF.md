# PROOF: lab/92-branchless-select

`bselect(sel, a, b)`: returns `b` if `sel == 1`, `a` if `sel == 0`,
for `uint64_t` `sel`, `a`, `b`, with no comparison and no branch in
the implementation. Contract: `sel` must be 0 or 1.

## What was built

`select.h`, `select.c`, `test_select.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics,
no builtins, no library math. Toolchain: gcc 13.3.0 on x86_64.

The implementation:

```c
uint64_t bselect(uint64_t sel, uint64_t a, uint64_t b)
{
    uint64_t mask = (uint64_t)(-(uint64_t)sel);
    return (a & ~mask) | (b & mask);
}
```

## Hand derivation

Work entirely in the unsigned domain, where every operation is total.

Fact 1: unsigned negation wraps modulo 2^64 (C11 6.2.5p9), so for
every `uint64_t x`, `-x = 2^64 - x` exactly, including `-0 = 0`. For
`sel` in `{0, 1}`:

| sel | -(uint64_t)sel        | mask               |
|-----|----------------------|--------------------|
| 0   | 2^64 - 0 = 0         | 0x0000000000000000 |
| 1   | 2^64 - 1             | 0xFFFFFFFFFFFFFFFF |

No signed arithmetic is performed, so no overflow and no undefined
behavior are possible for any input, including `sel = UINT64_MAX`.

Fact 2: AND/OR/NOT act per bit, and the two contract masks are
complements, so the halves of `(a & ~mask) | (b & mask)` cover every
bit exactly once and disjointly:

| sel | ~mask                | (a & ~mask) | (b & mask) | out |
|-----|----------------------|-------------|------------|-----|
| 0   | all bits 1           | a           | 0          | a   |
| 1   | all bits 0           | 0           | b          | b   |

Hence `bselect` returns `a` for `sel = 0` and `b` for `sel = 1`, for
all 2^128 `(a, b)` pairs.

Out-of-contract behavior, stated explicitly: for `sel` outside
`{0, 1}` the mask is some other value and the output is the raw
arithmetic blend, not a defined selection. Examples, all printed by
the test program:

- `sel = 2`: `mask = 2^64 - 2 = 0xFFFFFFFFFFFFFFFE`;
  `bselect(2, 0, UINT64_MAX) = 0xFFFFFFFFFFFFFFFE`.
- `sel = 3`: `mask = 0xFFFFFFFFFFFFFFFD`;
  `bselect(3, 0xAAAAAAAAAAAAAAAA, 0x5555555555555555) = 0x5555555555555557`.
- `sel = UINT64_MAX`: `mask = 1`;
  `bselect(UINT64_MAX, 0, UINT64_MAX) = 1`.
- `sel = 0x8000000000000000`: `mask = 0x8000000000000000`;
  `bselect(0x8000..., UINT64_MAX, 0) = 0x7FFFFFFFFFFFFFFF`.

These rows are never differential-checked and never enter the
checksum. They exist only to make the contract boundary explicit.

## Verification plan

1. Directed rows: 9 in-contract triples (sel 0/1, a/b in
   {0, UINT64_MAX, 0xDEADBEEFDEADBEEF, 0x1234567812345678}), each
   checked against the ternary reference `sel ? b : a` and printed.
2. Differential test against that reference over all 8,589,934,592
   16-bit `(sel, a, b)` triples (`sel` in `{0, 1}`, `a` and `b` over
   `[0, 65535]`) plus 10,000,000 fixed-seed splitmix64 64-bit triples
   (seed `0x123456789ABCDEF0`).
3. FNV-1a 64-bit checksum over the entire result stream must be
   identical across -O0, -O2, and ASan+UBSan builds.
4. -O2 disassembly of the object must contain no jump instruction.
5. Throughput at -O2, best of 5.

## Genuine build log

```
$ make test_select test_select_O0 test_select_san bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_select test_select.c select.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_select_O0 test_select.c select.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_select_san test_select.c select.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_select_bench test_select.c select.c
```

Zero warnings on all four lines. Zero ASan/UBSan reports at runtime
on all binaries.

## Genuine run output (-O2, -O0, ASan+UBSan)

All three binaries printed the same result:

```
directed rows:
  sel=0 a=0000000000000000 b=0000000000000000 bselect=0000000000000000 ref=0000000000000000 ok
  sel=0 a=0000000000000000 b=ffffffffffffffff bselect=0000000000000000 ref=0000000000000000 ok
  sel=0 a=ffffffffffffffff b=0000000000000000 bselect=ffffffffffffffff ref=ffffffffffffffff ok
  sel=0 a=deadbeefdeadbeef b=1234567812345678 bselect=deadbeefdeadbeef ref=deadbeefdeadbeef ok
  sel=1 a=0000000000000000 b=0000000000000000 bselect=0000000000000000 ref=0000000000000000 ok
  sel=1 a=0000000000000000 b=ffffffffffffffff bselect=ffffffffffffffff ref=ffffffffffffffff ok
  sel=1 a=ffffffffffffffff b=0000000000000000 bselect=0000000000000000 ref=0000000000000000 ok
  sel=1 a=deadbeefdeadbeef b=1234567812345678 bselect=1234567812345678 ref=1234567812345678 ok
  sel=1 a=ffffffffffffffff b=ffffffffffffffff bselect=ffffffffffffffff ref=ffffffffffffffff ok
out-of-contract rows (sel not in {0,1}, shown, not checked):
  sel=2 mask=fffffffffffffffe a=0000000000000000 b=ffffffffffffffff bselect=fffffffffffffffe
  sel=3 mask=fffffffffffffffd a=aaaaaaaaaaaaaaaa b=5555555555555555 bselect=5555555555555557
  sel=18446744073709551615 mask=0000000000000001 a=0000000000000000 b=ffffffffffffffff bselect=0000000000000001
  sel=9223372036854775808 mask=8000000000000000 a=ffffffffffffffff b=0000000000000000 bselect=7fffffffffffffff
checks=8599934601 mismatches=0 fnv1a=47f9a66e9c481f08
```

8,599,934,601 = 9 directed + 8,589,934,592 exhaustive +
10,000,000 random. 0 mismatches. FNV-1a checksum `47f9a66e9c481f08`
is identical across -O0, -O2, and the ASan+UBSan build (all three
binaries were run and printed the same checksum line).

## Genuine benchmark output

```
bench: 2.68 ns/value (372.5 Mvalues/s over 25M timed values, best of 5)
```

Loop conditions, stated honestly: the 1M triples were generated once
into a malloc'd array before timing; the timed region is 25 passes
over that array, XOR-ing each `bselect` result into a `volatile`
sink. What is measured is `bselect` plus loop and memory traffic, not
the RNG. Compiled with `-O2 -DBENCH` as shown in the build log.

## -O2 disassembly of bselect

`gcc -std=c11 -Wall -Wextra -Werror -O2 -c select.c`, `objdump -d`:

```
0000000000000000 <bselect>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 8d 47 ff         	lea    -0x1(%rdi),%rax
   8:	48 f7 df            	neg    %rdi
   b:	48 21 f0            	and    %rsi,%rax
   e:	48 21 d7            	and    %rdx,%rdi
  11:	48 09 f8            	or     %rdi,%rax
  14:	c3                  	ret
```

A programmatic scan of the function's disassembly found 0 jump
instructions (`grep -cE '\sj[a-z]+'` on the `bselect` block returned
0). The mnemonic set is exactly `endbr64`, `lea`, `neg`, `and`,
`and`, `or`, `ret`: `neg` builds the mask from `sel` (`lea -1`
holds the `~mask` side, a strength reduction the compiler chose),
the two `and`s form the disjoint halves, `or` combines them. No
`cmov` was even needed.

## What was verified, exactly

- The identity `bselect(sel, a, b) == (sel ? b : a)` held for every
  one of the 8,589,934,592 16-bit `(sel, a, b)` triples, 10,000,000
  fixed-seed 64-bit triples, and the 9 directed rows, 0 mismatches.
- The contract `sel` in `{0, 1}` is documented in `select.h`,
  derived in `select.c`, and exercised by 4 printed out-of-contract
  rows that are never differential-checked: callers cannot mistake
  the blend result for a selection.
- No signed arithmetic appears in the implementation, so no
  undefined behavior exists (also confirmed by the UBSan build
  running clean).
- The result stream is bit-identical across -O0, -O2, and
  ASan+UBSan (FNV-1a `47f9a66e9c481f08`).
- The -O2 object code contains no jump instruction.
- Throughput: 2.68 ns/value, 372.5 Mvalues/s, best of 5, under the
  loop conditions stated above.

All test binaries exited 0.
