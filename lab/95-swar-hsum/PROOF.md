<!-- PROOF-HEADER
Checks: 4304967310
Mismatches: 0
Checksum: 0eb4f4d7484f98bd
Throughput: 2.21 ns/value at -O2, best of 5
Environment: Host
-->
# PROOF: lab/95-swar-hsum

`swar_hsum(w)`: horizontal sum of the four packed unsigned 16-bit
lanes of a 64-bit word. Lane 0 is bits [15:0], lane 1 is [31:16],
lane 2 is [47:32], lane 3 is [63:48]. The result is at most
4*65535 = 262140, returned as `uint32_t`. Defined for every
`uint64_t` input; there is no out-of-contract input.

## What was built

`hsum.h`, `hsum.c`, `test_hsum.c`, `Makefile`, `README.md`, this file.
Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no
builtins, no library math. Toolchain: gcc 13.3.0 on x86_64.

The implementation, from the pairwise-add cascade identity:

```c
uint32_t swar_hsum(uint64_t w)
{
    uint64_t a = w & 0x0000FFFF0000FFFFULL;
    uint64_t b = (w >> 16) & 0x0000FFFF0000FFFFULL;
    uint64_t t = a + b;
    uint64_t lo = t & 0xFFFFFFFFULL;
    uint64_t hi = t >> 32;
    return (uint32_t)(lo + hi);
}
```

Stage 1 sums the even lanes (0, 2) with the odd lanes (1, 3) shifted
down into the even positions, so lane 0 of `t` holds lane0+lane1 and
lane 2 of `t` holds lane2+lane3. Stage 2 folds the two 32-bit halves
into the final sum.

## Hand derivation, with the guard-bound crosstalk argument

Work entirely in the unsigned domain, where every operation is total.

Fact 1: unsigned addition is addition modulo 2^64 (C11 6.2.5p9), so
whenever the true integer sum fits below 2^64 the result is the exact
integer sum. Each lane holds at most 0xFFFF, so each pair sum is at
most 0xFFFF + 0xFFFF = 0x1FFFE = 131070, which fits in 17 bits.

Fact 2, stage 1: `a` holds lanes 0 and 2 in 32-bit fields (bits 16..31
and 48..63 of each field are zero guard bits); `b` holds lanes 1 and
3 in the same fields. Consider the low 32-bit field: its sum is
lane0 + lane1, at most 0x1FFFE, strictly less than 2^32. Because the
field sum cannot reach 2^32, it produces no carry into bit 32; the
guard bits above it (16..31) can hold only bit 16 of the pair sum and
are otherwise zero. The same holds for the upper field (lane2 +
lane3). Hence bits 48..63 of `t` are all zero, bits [31:0] of `t` are
exactly lane0+lane1, and bits [47:32] of `t` are exactly lane2+lane3.
No bit of one pair sum can enter the other pair's field: the guard
bound is the inequality "pair sum < 2^32", proved from the lane bound
0xFFFF. That inequality is the complete crosstalk-absence proof for
stage 1: carries cannot cross a guard region that the integer sum
cannot reach.

Fact 3, stage 2: `lo = t & 0xFFFFFFFF` equals lane0+lane1 (at most
0x1FFFE); `hi = t >> 32` equals lane2+lane3 (at most 0x1FFFE). Their
sum is at most 0x3FFFC, below 2^32, so the addition is exact in the
low 32 bits with no carry out. The returned `uint32_t` is therefore
(lane0+lane1) + (lane2+lane3), the exact lane sum.

Nothing here is approximate, no signed arithmetic is performed, and
the worst case (all lanes 0xFFFF, sum 0x3FFFC) is covered by the same
bounds.

## Verification plan

1. Directed rows: 14 words covering the zero word, the all-ones word
   (worst case 0x3FFFC), each lane alone at maximum, adjacent pair
   boundaries, alternating patterns, the case where all lanes are
   0x8000 (pair sums sit at exactly 0x10000, one past the 16-bit
   boundary, the sharpest guard-bound case), and arbitrary patterns;
   each printed and differential-checked against the scalar
   lane-shift-and-add reference in `test_hsum.c`.
2. Differential test against that reference over all 2^32 words of
   the form (a, b, a, b), i.e. every 16-bit lane pair replicated to
   all four lanes, plus 10,000,000 fixed-seed splitmix64 random
   64-bit words (seed `0x123456789ABCDEF0`).
3. FNV-1a 64-bit checksum over the entire result stream must be
   identical across -O0, -O2, and ASan+UBSan builds.
4. Throughput at -O2, best of 5, with the PRNG outside the timed
   loop (1M words generated once before timing, 25 passes timed).

## Genuine build log

Captured 2026-09-10, gcc 13.3.0 on x86_64, zero warnings under
`-Wall -Wextra -Werror`:

```
$ make test_hsum test_hsum_O0 test_hsum_san bench
gcc -std=c11 -Wall -Wextra -Werror -O2 -o test_hsum test_hsum.c hsum.c
gcc -std=c11 -Wall -Wextra -Werror -O0 -o test_hsum_O0 test_hsum.c hsum.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -fsanitize=address,undefined -fno-sanitize-recover=all -o test_hsum_san test_hsum.c hsum.c
gcc -std=c11 -Wall -Wextra -Werror -O2 -DBENCH -o test_hsum_bench test_hsum.c hsum.c
```

## Genuine run output, -O2 build

```
directed rows:
  w=0000000000000000 swar_hsum=00000000 ref=00000000 ok
  w=ffffffffffffffff swar_hsum=0003fffc ref=0003fffc ok
  w=000000000000ffff swar_hsum=0000ffff ref=0000ffff ok
  w=00000000ffff0000 swar_hsum=0000ffff ref=0000ffff ok
  w=0000ffff00000000 swar_hsum=0000ffff ref=0000ffff ok
  w=ffff000000000000 swar_hsum=0000ffff ref=0000ffff ok
  w=00000000ffffffff swar_hsum=0001fffe ref=0001fffe ok
  w=ffffffff00000000 swar_hsum=0001fffe ref=0001fffe ok
  w=0001000100010001 swar_hsum=00000004 ref=00000004 ok
  w=8000800080008000 swar_hsum=00020000 ref=00020000 ok
  w=fffefffefffefffe swar_hsum=0003fff8 ref=0003fff8 ok
  w=0001000200030004 swar_hsum=0000000a ref=0000000a ok
  w=a5a55a5aa5a55a5a swar_hsum=0001fffe ref=0001fffe ok
  w=123456789abcdef0 swar_hsum=0001e258 ref=0001e258 ok
checks=4304967310 mismatches=0 fnv1a=0eb4f4d7484f98bd
```

## Genuine run summaries, -O0 and ASan+UBSan builds

Both the -O0 build and the ASan+UBSan build printed the same 14
directed rows, all `ok`, and:

```
checks=4304967310 mismatches=0 fnv1a=0eb4f4d7484f98bd
```

No sanitizer findings were reported by the ASan+UBSan build.

## Checksum identity across builds

FNV-1a `0eb4f4d7484f98bd` is identical across the -O2, -O0, and
ASan+UBSan runs over the same 4,304,967,310-result stream.

## Genuine throughput measurement (-DBENCH build, -O2)

```
checks=4304967310 mismatches=0 fnv1a=0eb4f4d7484f98bd
bench: 2.21 ns/value (452.1 Mvalues/s over 25M timed values, best of 5)
```

The 1M-word input array was generated once before timing; the timed
region is 25 passes over that array, so the PRNG is not in the timed
loop. The measured cost includes `swar_hsum` plus loop overhead and
memory traffic.

Run `make run` in this directory to reproduce. The build log above
and these outputs are the genuine artifacts of that procedure; no
output was edited or fabricated.
