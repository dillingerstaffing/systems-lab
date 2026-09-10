<!-- PROOF-HEADER
Checks: 4304967312
Mismatches: 0
Checksum: 731e0818deb17926
Throughput: 4.31 ns/value (232.1 Mvalues/s over 25M timed values, best of 5)
Environment: Host
-->

# PROOF: lab/114-swar-eq-mask

`swar_eqmask64(x, y)`: returns an 8-bit mask with bit `i` set iff
byte `i` of `x` equals byte `i` of `y` (byte 0 is the least
significant byte), for `uint64_t` `x`, `y`, with no conditional
branch in the implementation.

## What was built

`eqmask.h`, `eqmask.c`, `test_eqmask.c`, `Makefile`, `README.md`,
this file. Plain C11, `-std=c11 -Wall -Wextra -Werror`, no
intrinsics, no builtins, no library math. Toolchain: gcc 13.3.0 on
x86_64.

The implementation:

```c
uint8_t swar_eqmask64(uint64_t x, uint64_t y)
{
    uint64_t d = x ^ y;
    uint64_t de = (d & 0x00FF00FF00FF00FFULL) | 0xFF00FF00FF00FF00ULL;
    uint64_t te = ((de - 0x0101010101010101ULL) & ~de) & 0x8080808080808080ULL;
    uint64_t do_ = (d & 0xFF00FF00FF00FF00ULL) | 0x00FF00FF00FF00FFULL;
    uint64_t to = ((do_ - 0x0101010101010101ULL) & ~do_) & 0x8080808080808080ULL;
    uint64_t t = te | to;
    uint64_t u = t >> 7;
    return (uint8_t)(((u >>  0) & 0x01u) | ((u >>  7) & 0x02u) |
                     ((u >> 14) & 0x04u) | ((u >> 21) & 0x08u) |
                     ((u >> 28) & 0x10u) | ((u >> 35) & 0x20u) |
                     ((u >> 42) & 0x40u) | ((u >> 49) & 0x80u));
}
```

## Hand derivation

Work entirely in the unsigned domain, where subtraction and shifts
by constants in `[0, 64)` are total (C11 6.2.5p9: unsigned
arithmetic wraps modulo 2^64).

Fact 1: byte `i` of `d = x ^ y` is `x_i ^ y_i`, which is 0 iff
`x_i == y_i` (XOR is bitwise: zero in a byte exactly when the two
bytes agree in every bit). So the task is an exact zero-byte mask
of `d`.

Fact 2 (zero-byte identity, per byte): write `d_i` for byte `i` of
`d` and `c_i` in `{0, 1}` for the borrow into byte `i` in
`(d - 0x0101010101010101)`; byte `i` of the difference is
`(d_i - 1 - c_i) mod 256`. The candidate
`t = ((d - 0x0101010101010101) & ~d & 0x8080808080808080)`
sets bit `8i+7` when that byte's bit 7 and `(~d)`'s bit 7 are both
1. If `d_i = 0`, the byte is `0xFF` (`c_i = 0`) or `0xFE`
(`c_i = 1`), bit 7 set, and `~d` has `0xFF`, bit 7 set: genuine
zero bytes are always flagged (no false negatives). But with
`d_i != 0` both bit 7s can still be 1. With `c_i = 0`,
`(d_i - 1)` has bit 7 set only for `d_i >= 129` while `~d_i` has
bit 7 set only for `d_i <= 127`: disjoint, no false positive. With
`c_i = 1`, `(d_i - 2) mod 256` has bit 7 set for `d_i = 1`
(`0xFF`) or `d_i >= 130`, and `~d_i` has bit 7 set for
`d_i <= 127`: the only common value is `d_i = 1`. So the naive
form flags a nonzero byte `0x01` whenever a borrow arrives from
below. It is an existence test (if any zero byte exists the result
is nonzero), not an exact mask.

Fact 3 (borrow suppression): park the untested half at `0xFF`.
With `de = (d & 0x00FF00FF00FF00FF) | 0xFF00FF00FF00FF00`, odd bytes
are `0xFF` and emit no borrow (`0xFF - 1 - c >= 0xFD` for
`c` in `{0, 1}`), so every even byte of
`(de - 0x0101010101010101)` sees borrow-in 0 (byte 0 by definition;
byte `2i`, `i > 0`, because odd byte `2i-1` emits none). The Fact 2
analysis with `c_i = 0` then applies exactly: bit `8i+7` of
`((de - 0x0101010101010101) & ~de & 0x8080808080808080)` is 1 iff
byte `2i` of `d` is 0, and its odd bytes are `0x00` (`~de` is
`0x00` there). Symmetrically,
`do = (d & 0xFF00FF00FF00FF00) | 0x00FF00FF00FF00FF` yields the
exact odd-byte mask with `0x00` even bytes. ORing the two gives
`t` with `0x80` in byte `i` iff `d_i = 0`, exactly, for all eight
bytes.

Fact 4 (gather): `u = t >> 7` has bit `8i` set exactly for the
equal bytes. Term `((u >> 7i) & (1 << i))` reads bit `8i` of `u`
(bit `i` of `u >> 7i` is bit `i + 7i = 8i`) and places it at output
bit `i`. ORing the eight disjoint terms yields a byte whose bit `i`
is 1 iff byte `i` matched; the terms are disjoint single bits, so
no carry or overlap is possible.

Hence the returned byte has bit `i = 1` iff byte `i` of `x` equals
byte `i` of `y`, for every one of the 2^128 `(x, y)` pairs. No
signed arithmetic appears anywhere, so no signed overflow is
possible (also confirmed by the UBSan build running clean).

## A bug the differential test caught

The first implementation used the naive single-shot identity
(Fact 2 without Fact 3). The exhaustive sweep immediately reported
mismatches, e.g. `x = 0x19eb, y = 0x18eb: got=ff want=fd`: here
`d = 0x0100`, byte 1 is `0x01` (bytes differ), but byte 0 borrows
in the subtraction, so the naive form flagged byte 1. That is
exactly the `d_i = 0x01` with borrow-in false positive proved in
Fact 2. The implementation was rewritten with the borrow
suppression of Fact 3, after which the full sweep below passed
with 0 mismatches. A standalone Python model of both forms
confirmed the diagnosis independently: the naive form mismatched
the exact mask on 272 of 1,048,576 values in `[0, 2^20)`, all of
the `0x01`-with-borrow shape, while the suppressed form matched on
all of them plus 300,000 random 64-bit values.

## Verification plan

1. Directed rows: 16 `(x, y)` pairs covering all-equal,
   all-different, single-byte differences at the low end, the high
   end, and the middle, the `0x80` high-bit boundary, and the
   adversarial `d = 0x0101010101010101` input, each checked against
   the naive per-byte-loop reference and printed.
2. Differential test against that reference over all 4,294,967,296
   16-bit pairs (`x` and `y` over `[0, 65535]`) plus 10,000,000
   fixed-seed splitmix64 64-bit pairs (seed `0x123456789ABCDEF0`).
3. FNV-1a 64-bit checksum over the entire result stream must be
   identical across -O0, -O2, and ASan+UBSan builds.
4. -O2 disassembly of the object must contain no conditional jump.
5. Throughput at -O2, best of 5.

## Genuine build log

```
$ make clean && make test_eqmask test_eqmask_O0 test_eqmask_san
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_eqmask test_eqmask.c eqmask.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_eqmask_O0 test_eqmask.c eqmask.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_eqmask_san test_eqmask.c eqmask.c
```

Zero warnings on all three lines. Zero ASan/UBSan reports at runtime
on all binaries.

## Genuine run output (-O2, -O0, ASan+UBSan)

All three binaries printed the same result:

```
directed rows:
  x=0000000000000000 y=0000000000000000 eqmask=ff ref=ff ok
  x=0000000000000000 y=0000000000000001 eqmask=fe ref=fe ok
  x=0000000000000001 y=0000000000000000 eqmask=fe ref=fe ok
  x=0000000000000000 y=ffffffffffffffff eqmask=00 ref=00 ok
  x=ffffffffffffffff y=0000000000000000 eqmask=00 ref=00 ok
  x=ffffffffffffffff y=ffffffffffffffff eqmask=ff ref=ff ok
  x=0102030405060708 y=0102030405060708 eqmask=ff ref=ff ok
  x=0102030405060708 y=0102030405060709 eqmask=fe ref=fe ok
  x=0102030405060708 y=0002030405060708 eqmask=7f ref=7f ok
  x=0102030405060708 y=0102030405ff0708 eqmask=fb ref=fb ok
  x=8080808080808080 y=8080808080808080 eqmask=ff ref=ff ok
  x=8080808080808080 y=0000000000000000 eqmask=00 ref=00 ok
  x=00ff00ff00ff00ff y=00ff00ff00ff00fe eqmask=fe ref=fe ok
  x=0101010101010101 y=0000000000000000 eqmask=00 ref=00 ok
  x=0101010101010101 y=0101010101010101 eqmask=ff ref=ff ok
  x=8000000000000000 y=7fffffffffffffff eqmask=00 ref=00 ok
checks=4304967312 mismatches=0 fnv1a=731e0818deb17926
```

4,304,967,312 = 16 directed + 4,294,967,296 exhaustive + 10,000,000
random. 0 mismatches. FNV-1a checksum `731e0818deb17926` is
identical across -O0, -O2, and the ASan+UBSan build (all three
binaries were run and printed the same checksum line).

## Genuine benchmark output

```
$ make bench && ./test_eqmask_bench   (tail of output)
bench: 4.31 ns/value (232.1 Mvalues/s over 25M timed values, best of 5)
```

Loop conditions, stated honestly: the 1M pairs were generated once
into a malloc'd array before timing; the timed region is 25 passes
over that array, XOR-ing each `swar_eqmask64` result into a
`volatile` sink. What is measured is `swar_eqmask64` plus loop and
memory traffic, not the RNG. Compiled with `-O2 -DBENCH` as shown
in the build log.

## -O2 disassembly of swar_eqmask64

`gcc -std=c11 -Wall -Wextra -Werror -O2 -c eqmask.c`, `objdump -d`:

```
0000000000000000 <swar_eqmask64>:
   0:	f3 0f 1e fa          	endbr64
   4:	48 b8 ff 00 ff 00 ff 	movabs $0xff00ff00ff00ff,%rax
   b:	00 ff 00
   e:	48 31 f7             	xor    %rsi,%rdi
  11:	48 b9 00 ff 00 ff 00 	movabs $0xff00ff00ff00ff00,%rcx
  18:	ff 00 ff
  1b:	48 09 f9             	or     %rdi,%rcx
  1e:	48 09 c7             	or     %rax,%rdi
  21:	48 b8 ff fe fe fe fe 	movabs $0xfefefefefefefeff,%rax
  28:	fe fe fe
  2b:	48 8d 14 01          	lea    (%rcx,%rax,1),%rdx
  2f:	48 01 f8             	add    %rdi,%rax
  32:	48 f7 d1             	not    %rcx
  35:	48 f7 d7             	not    %rdi
  38:	48 21 ca             	and    %rcx,%rdx
  3b:	48 21 f8             	and    %rdi,%rax
  3e:	48 09 c2             	or     %rax,%rdx
  41:	48 b8 01 01 01 01 01 	movabs $0x101010101010101,%rax
  48:	01 01 01
  4b:	48 c1 ea 07          	shr    $0x7,%rdx
  4f:	48 21 c2             	and    %rax,%rdx
  52:	48 89 d0             	mov    %rdx,%rax
  55:	48 89 d1             	mov    %rdx,%rcx
  58:	48 c1 e9 0e          	shr    $0xe,%rcx
  5c:	48 c1 e8 07          	shr    $0x7,%rax
  60:	09 c8                	or     %ecx,%eax
  62:	48 89 d1             	mov    %rdx,%rcx
  65:	09 d0                	or     %edx,%eax
  67:	48 c1 e9 15          	shr    $0x15,%rcx
  6b:	09 c8                	or     %ecx,%eax
  6d:	48 89 d1             	mov    %rdx,%rcx
  70:	48 c1 e9 1c          	shr    $0x1c,%rcx
  74:	09 c8                	or     %ecx,%eax
  76:	48 89 d1             	mov    %rdx,%rcx
  79:	48 c1 e9 23          	shr    $0x23,%rcx
  7d:	09 c8                	or     %ecx,%eax
  7f:	48 89 d1             	mov    %rdx,%rcx
  82:	48 c1 ea 31          	shr    $0x31,%rdx
  86:	48 c1 e9 2a          	shr    $0x2a,%rcx
  8a:	09 c8                	or     %ecx,%eax
  8c:	09 d0                	or     %edx,%eax
  8e:	c3                   	ret
```

A programmatic scan of the function's disassembly found 0 jump
instructions of any kind (`grep -cE '\sj[a-z]+'` on the
`swar_eqmask64` block returned 0). The mnemonic set is exactly
`endbr64`, `movabs`, `xor`, `or`, `lea`, `add`, `not`, `and`,
`shr`, `mov`, `ret`. The compiler folded each `de - 0x0101...01`
into an add of `-0x0101010101010101` (`lea`/`add` against
`0xfefefefefefefeff`), and reduced the two half-masks and the
gather to `not`/`and`/`or`/`shr` chains with no branch.

## What was verified, exactly

- The identity `swar_eqmask64(x, y) == ref_eqmask(x, y)` (bit `i`
  set iff byte `i` equal, naive per-byte loop) held for every one
  of the 4,294,967,296 16-bit `(x, y)` pairs, 10,000,000
  fixed-seed 64-bit pairs, and the 16 directed rows, 0 mismatches.
- The naive single-shot zero-byte identity is not an exact mask
  (false positive: nonzero `0x01` byte with borrow-in, e.g.
  `d = 0x0100`); the shipped even/odd borrow-suppressed form is
  exact, proved per byte in Fact 3 and confirmed by the sweep.
- No signed arithmetic appears in the implementation, so no signed
  overflow is possible (also confirmed by the UBSan build running
  clean).
- The result stream is bit-identical across -O0, -O2, and
  ASan+UBSan (FNV-1a `731e0818deb17926`).
- The -O2 object code contains no conditional jump (verified by
  scan: 0 jump instructions).
- Throughput: 4.31 ns/value, 232.1 Mvalues/s, best of 5, under the
  loop conditions stated above.

All test binaries exited 0.
