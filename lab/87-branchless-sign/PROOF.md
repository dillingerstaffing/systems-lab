<!-- PROOF-HEADER
Checks: 1065541
Mismatches: 0
Checksum: 21a06a0e15a1874e
Throughput: 2.22 ns/value at -O2, best of 5
Environment: Host
-->
# PROOF: lab/87-branchless-sign

`sign64(x)`: -1 if `x < 0`, 0 if `x == 0`, +1 if `x > 0`, for every
`int64_t x`, with no comparison and no branch in the implementation.

## What was built

`sign.h`, `sign.c`, `test_sign.c`, `Makefile`, `README.md`, this
file. Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no
builtins, no library math. Toolchain: gcc 13.3.0 on x86_64.

The implementation:

```c
int64_t sign64(int64_t x)
{
    uint64_t ux = (uint64_t)x;
    int64_t neg = -(int64_t)(ux >> 63);
    int64_t pos = (int64_t)((0u - ux) >> 63);
    return neg | pos;
}
```

## Hand derivation

Work in the unsigned domain, where every operation is total.

Let `ux = (uint64_t)x`. By C11 6.3.1.3p2 the conversion preserves the
value modulo 2^64, so on a two's-complement machine `ux` is exactly
the bit pattern of `x`. By C11 6.5.7p5, `ux >> 63` is a logical shift
with zero fill, so it is the top bit of the pattern: 1 iff `x < 0`,
0 otherwise. Casting that bit to `int64_t` gives 0 or 1, and negating
gives `neg` = -1 (all bits set) for `x < 0`, 0 for `x >= 0`. No
overflow is possible: the negation applies to the value 0 or 1.

`pos`: `0u - ux` promotes the `0u` to `uint64_t`, and unsigned
subtraction wraps modulo 2^64 (C11 6.2.5p9), so this is the unsigned
value of `-x` for every `x`, including `INT64_MIN`, where the signed
negation `-x` would be undefined behavior. Its top bit:

| x                | (uint64_t)x | 0u - (uint64_t)x = 2^64 - (uint64_t)x | top bit |
|------------------|-------------|---------------------------------------|---------|
| x = 0            | 0           | 0                                     | 0       |
| x > 0            | in [1, 2^63-1] | in [2^63+1, 2^64-1]                | 1       |
| x < 0, x != INT64_MIN | in [2^63+1, 2^64-1] | in [1, 2^63-1]            | 0       |
| x = INT64_MIN    | 2^63        | 2^63                                  | 1       |

So `pos` is 1 exactly when `x > 0` (the INT64_MIN row is decided by
`neg` anyway). The two parts are disjoint: when `x < 0`, `neg` is -1,
all bits set, so `neg | pos = -1` regardless of `pos`; when
`x >= 0`, `neg` is 0 and the result is `pos`, which is 1 for `x > 0`
and 0 for `x = 0`. Hence the OR is -1, 0, or +1 exactly as required,
for all 2^64 inputs.

INT64_MIN contract, by hand: `ux = 0x8000000000000000`, so
`ux >> 63 = 1`, `neg = -1`. `0u - ux = 0x8000000000000000`,
`>> 63 = 1`, `pos = 1`. `-1 | 1 = -1`. Correct.

INT64_MAX contract, by hand: `ux = 0x7FFFFFFFFFFFFFFF`, top bit 0,
`neg = 0`. `0u - ux = 0x8000000000000001`, top bit 1, `pos = 1`.
`0 | 1 = 1`. Correct.

## Verification plan

1. Directed rows: INT64_MIN, -1, 0, 1, INT64_MAX, each checked
   against the comparison reference `(x > 0) - (x < 0)`.
2. Differential test against that reference over all 65,536 16-bit
   inputs (as int64) plus 1,000,000 fixed-seed splitmix64 64-bit
   values (seed 0x123456789ABCDEF0).
3. FNV-1a 64-bit checksum over the entire result stream must be
   identical across -O0, -O2, and ASan+UBSan builds.
4. -O2 disassembly of the object must contain no jump instruction.
5. Throughput at -O2, best of 5.

## Genuine build log

```
$ make
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_sign test_sign.c sign.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_sign_O0 test_sign.c sign.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_sign_san test_sign.c sign.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_sign_bench test_sign.c sign.c
```

Zero warnings. Zero ASan/UBSan reports at runtime.

## Genuine run output (-O2, -O0, ASan+UBSan)

All three binaries printed the same result:

```
directed rows:
  x=-9223372036854775808 sign64=-1 ref=-1 ok
  x=                  -1 sign64=-1 ref=-1 ok
  x=                   0 sign64= 0 ref= 0 ok
  x=                   1 sign64= 1 ref= 1 ok
  x= 9223372036854775807 sign64= 1 ref= 1 ok
checks=1065541 mismatches=0 fnv1a=21a06a0e15a1874e
```

1,065,541 = 5 directed + 65,536 exhaustive + 1,000,000 random.
0 mismatches. FNV-1a checksum `21a06a0e15a1874e` is identical across
-O0, -O2, and the ASan+UBSan build (all three binaries were run and
printed the same checksum line).

## Genuine benchmark output

```
bench: 2.22 ns/value (450.7 Mvalues/s over 25M timed values, best of 5)
```

Loop conditions, stated honestly: the 1M inputs were generated once
into a malloc'd array before timing; the timed region is 25 passes
over that array, XOR-ing each `sign64` result into a `volatile`
sink. What is measured is `sign64` plus loop and memory traffic, not
the RNG. Compiled with `-O2 -DBENCH` as shown in the build log.

## -O2 disassembly of sign64

```
0000000000000000 <sign64>:
   0:	f3 0f 1e fa         	endbr64
   4:	48 89 f8            	mov    %rdi,%rax
   7:	48 c1 ff 3f         	sar    $0x3f,%rdi
   b:	48 f7 d8            	neg    %rax
   e:	48 c1 e8 3f         	shr    $0x3f,%rax
  12:	48 09 f8            	or     %rdi,%rax
  15:	c3                  	ret
```

`gcc -std=c11 -Wall -Wextra -Werror -O2 -c sign.c`, `objdump -d`. The compiler fused the two
halves: `sar $63` on the original input yields the -1/0 `neg` part,
`neg` then `shr $63` on a copy yields the 0/1 `pos` part (top bit of
`0 - ux`), and `or` combines them. There is no conditional jump and
no jump instruction of any kind in the function: the mnemonic set is
`endbr64`, `mov`, `sar`, `neg`, `shr`, `or`, `ret`. No `cmov` was
even needed.

## What was verified, exactly

- The identity `sign64(x) == (x > 0) - (x < 0)` held for every
  16-bit input (65,536 values), 1,000,000 fixed-seed 64-bit values,
  and the 5 directed boundary values, 0 mismatches.
- INT64_MIN and INT64_MAX are covered both by the directed rows and
  by the hand proof above; no signed negation is ever performed, so
  no undefined behavior exists in the implementation (also confirmed
  by the UBSan build running clean).
- The result stream is bit-identical across -O0, -O2, and
  ASan+UBSan (FNV-1a `21a06a0e15a1874e`).
- The -O2 object code contains no jump instruction.
- Throughput: 2.22 ns/value, 450.7 Mvalues/s, best of 5, under the
  loop conditions stated above.

The run exited 0 on all three binaries and on the benchmark binary.
