<!-- PROOF-HEADER
Checks: 4304967440
Mismatches: 0
Throughput: 2.059 ns/pair at -O2
Verdict: PASS
-->
# PROOF.md — lab/59-avg-no-overflow

`avg_u64(a, b)`: floor of `(a + b) / 2` for two `uint64_t` values,
computed as `(a & b) + ((a ^ b) >> 1)` without ever forming `a + b`.
Header-only: `avg.h`. Tests: `test_avg.c`.

## Why the construction is exact

The integer equality `a + b = ((a & b) << 1) + (a ^ b)`: for each bit
position, the input bit pair contributes its sum; `a_i & b_i` is 1
exactly where the pair contributes 2, `a_i ^ b_i` exactly where it
contributes 1. Halving gives `(a + b) / 2 = (a & b) + (a ^ b) / 2`, and
since `(a & b)` is an integer, the floors satisfy
`floor((a + b) / 2) = (a & b) + floor((a ^ b) / 2) = (a & b) + ((a ^ b) >> 1)`.

The sum in the return statement equals `floor((a + b) / 2) <= 2^64 - 1`
exactly, so it cannot wrap. All operations are unsigned: no signed
overflow and no implementation-defined shift are possible anywhere.

## Oracle

The reference is the exact average: the true sum in
`unsigned __int128`, then halved. Any pair where `avg_u64` differs from
the oracle is a mismatch. Test phases:

- Phase 1: exhaustive over all 2^32 pairs of `uint16_t` operands
  (ran on the `-O2` build only).
- Phase 2: directed 144-pair edge sweep over
  `{0, 1, 2, 3, 2^32-1, 2^32, 0x5555555555555555, 0xAAAAAAAAAAAAAAAA,
  2^63-1, 2^63, 2^64-2, 2^64-1}`.
- Phase 3: 10,000,000 fixed-seed splitmix64 random 64-bit pairs (seed
  `0x243F6A8885A308D3`, fractional digits of pi; every run reproducible).

## Build

```
$ make clean && make
rm -f test_avg_o0 test_avg_o2 test_avg_asan
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O0 -o test_avg_o0 test_avg.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O2 -o test_avg_o2 test_avg.c
gcc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=199309L -O1 -g -fsanitize=address,undefined \
	-fno-omit-frame-pointer -o test_avg_asan test_avg.c
build exit=0
```

Clean under `-Wall -Wextra -Werror` at `-O0`, `-O2`, and
`-fsanitize=address,undefined`.

## Run

```
$ make run
./test_avg_o2 full
exhaustive: a=    0 of 65535, pairs=65536, mismatches=0
exhaustive: a= 8192 of 65535, pairs=536936448, mismatches=0
exhaustive: a=16384 of 65535, pairs=1073807360, mismatches=0
exhaustive: a=24576 of 65535, pairs=1610678272, mismatches=0
exhaustive: a=32768 of 65535, pairs=2147549184, mismatches=0
exhaustive: a=40960 of 65535, pairs=2684420096, mismatches=0
exhaustive: a=49152 of 65535, pairs=3221291008, mismatches=0
exhaustive: a=57344 of 65535, pairs=3758161920, mismatches=0
phase 1 (exhaustive 16-bit pairs): 4294967296 pairs, 0 mismatches, 8.8 s (2.059 ns/pair)
phase 2 (directed 64-bit edges): 144 pairs, 0 mismatches
phase 3 (10M random 64-bit pairs): 10000000 pairs, 0 mismatches
correctness: 4304967440 differential checks, 0 mismatches
ALL TESTS PASSED
./test_avg_o0
phase 2 (directed 64-bit edges): 144 pairs, 0 mismatches
phase 3 (10M random 64-bit pairs): 10000000 pairs, 0 mismatches
correctness: 10000144 differential checks, 0 mismatches
ALL TESTS PASSED
./test_avg_asan
phase 2 (directed 64-bit edges): 144 pairs, 0 mismatches
phase 3 (10M random 64-bit pairs): 10000000 pairs, 0 mismatches
correctness: 10000144 differential checks, 0 mismatches
ALL TESTS PASSED
run exit=0
```

## What was verified

- 4,304,967,440 differential checks on the `-O2` build (4,294,967,296
  exhaustive 16-bit pairs + 144 directed 64-bit edge pairs + 10,000,000
  random 64-bit pairs): 0 mismatches against the `unsigned __int128`
  exact reference.
- 10,000,144 differential checks each on the `-O0` and
  `-fsanitize=address,undefined` builds (144 edges + 10,000,000 random):
  0 mismatches, no sanitizer reports.
- Throughput of the differential check loop measured on the exhaustive
  sweep: 2.059 ns/pair (8.8 s for 2^32 pairs).

Nothing was verified beyond this: the exhaustive sweep covers only
16-bit operands; full 64-bit coverage comes from the 144 edge pairs
and the 10M random pairs.
