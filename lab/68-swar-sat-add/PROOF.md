<!-- PROOF-HEADER
Checks: 4294967296
Mismatches: 0
Checksum: 8de5b49f4d62f90d
Throughput: 4.589 ns/op at -O2
-->
# PROOF.md: lab/68-swar-sat-add

Environment: gcc 13.3.0 (Ubuntu), x86_64.

`swar_sat_add16x4()` (swar_sat_add.h) adds two `uint64_t` words as four
independent unsigned 16-bit lanes with saturation: a lane whose true
sum exceeds `0xFFFF` yields `0xFFFF`.  The construction splits even
lanes (0, 2) and odd lanes (1, 3) into two 32-bit-spaced pairs.  In each
pair a lane sum occupies at most 17 bits (`0xFFFF + 0xFFFF = 0x1FFFE`),
so the carry-out of a 16-bit overflow lands on a guard bit (bit 16 or
48 of the pair) that belongs to no lane.  The guard bits are isolated
with the mask `0x000100000001` after shifting right 16, broadcast to
full 16-bit lane masks by multiplying with `0xFFFF`, and select between
the raw lane sum and `0xFFFF` branchlessly.  Odd lanes are shifted down
16 first so lane 3 (the top lane, whose carry would otherwise fall off
the top of the word) also gets a guard bit, then shifted back.  All
arithmetic is unsigned 64-bit; no shift reaches 64; no UB.

## Hand derivations for the known-answer vectors

Lanes are listed low to high (lane 0 first).  Scalar reference: per
lane, `s = x + y`, result `s > 0xFFFF ? 0xFFFF : s`.

- `zero + zero`: every lane `0 + 0 = 0`, result `0x0000000000000000`.
- `all lanes saturate`: every lane `0xFFFF + 0xFFFF = 0x1FFFE`,
  clamps, result `0xFFFFFFFFFFFFFFFF`.
- `no lane overflows`: `a = {4,3,2,1}`, `b = {1,2,3,4}`, each lane sums
  to 5, result `0x0005000500050005`.
- lanes 0,2,3 saturate: `a = {0x0000,0xFFFF,0x0000,0xFFFF}`,
  `b = {0xFFFF,0x0001,0xFFFF,0x0001}`.  Lanes 0,2: `0xFFFF` exact;
  lanes 1,3: `0x10000`, clamp.  Result `0xFFFFFFFFFFFFFFFF`.
- lane2 saturates, neighbors untouched (crosstalk):
  `a = {0x0000,0x0000,0xFFFF,0x0000}`,
  `b = {0x0001,0x0001,0x0001,0x0001}`.  Lane 2: `0xFFFF + 0x0001 =
  0x10000`, clamps to `0xFFFF`; lanes 0,1,3: `0x0001` exact.  Result
  `0x0001FFFF00010001`.
- `0x8000+0x8000 saturates every lane`: `0x10000` per lane, clamp,
  result `0xFFFFFFFFFFFFFFFF`.
- `0x8000+0x7FFF = 0xFFFF exact`: per lane `0xFFFF`, no clamp,
  result `0xFFFFFFFFFFFFFFFF`.
- `lane0 0xFFFF+1 clamps`: `a = {0xFFFF,0,0,0}`,
  `b = {0x0001,0,0,0}`.  Lane 0: `0x10000`, clamp; rest 0.  Result
  `0x000000000000FFFF`.
- `top lane saturates, no carry lost`: `a = {0,0,0,0xFFFF}`,
  `b = {0,0,0,0x0001}`.  Lane 3: `0x10000`, clamp.  This exercises the
  shifted odd-lane path: lane 3's carry is captured by its guard bit
  instead of falling off the word.  Result `0xFFFF000000000000`.
- `lane1 saturates beside quiet lanes`: `a = {0,0xFFFF,0,0}`,
  `b = {0,0x0001,0x0001,0}`.  Lane 1: `0x10000`, clamp; lane 2:
  `0x0001`; rest 0.  Result `0x00000001FFFF0000`.

## Build log

```
$ make clean && make all
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_swar_sat_add test_swar_sat_add.c

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_swar_sat_add_asan test_swar_sat_add.c
./test_swar_sat_add_asan quick
known-answer vectors: 10 checked
directed crosstalk: 1679616 checks
fixed-seed random slice: 1048576 checks
throughput: 4.652 ns/op (214.9 Mops/s), N=100000000, acc=eb546b16c9dc80b4
fnv1a checksum: a2f2b90587bcb5b1
TOTAL mismatches: 0

$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_swar_sat_add_o0 test_swar_sat_add.c
./test_swar_sat_add_o0 quick
known-answer vectors: 10 checked
directed crosstalk: 1679616 checks
fixed-seed random slice: 1048576 checks
throughput: 7.538 ns/op (132.7 Mops/s), N=100000000, acc=eb546b16c9dc80b4
fnv1a checksum: a2f2b90587bcb5b1
TOTAL mismatches: 0
```

## Full run (-O2, exhaustive)

```
$ ./test_swar_sat_add full
known-answer vectors: 10 checked
directed crosstalk: 1679616 checks
exhaustive lane pairs: 4294967296 checks
throughput: 4.589 ns/op (217.9 Mops/s), N=100000000, acc=eb546b16c9dc80b4
fnv1a checksum: 8de5b49f4d62f90d
TOTAL mismatches: 0
```

The `-O2` quick-mode run (directed sweep plus fixed-seed 2^20 slice,
same inputs as the `-O0` and ASan+UBSan runs) also produced checksum
`a2f2b90587bcb5b1`, identical across all three build configurations.
The throughput loop builds two pseudo-random words per iteration with
an LCG, runs the SWAR add, and folds the result into an accumulator, so
the measured time covers packing plus the op itself; it is reported as
measured, not as a claim about the op in isolation.

Note on the exhaustive loop: each of the 2^32 lane pairs is packed
identically into all four lanes, so every lane position sees every
possible 16-bit input pair (4 x 2^32 lane checks in one pass).  Lane
interaction beyond the boundary set is covered by the directed
crosstalk sweep (all 36^4 combinations of boundary lane values), which
is exactly the region where guard-bit or carry contamination could
hide.
