<!-- PROOF-HEADER
Checks: 1065792
Mismatches: 0
Checksum: 0xe1fc995406ad5592
Throughput: 7.405 ns/value at -O2 (mean of 5 passes)
Environment: Host
-->
# PROOF.md: lab/65-popcount-byte

Environment: gcc 13.3.0 (Ubuntu), x86_64, run 2026-09-09.

`popcount64()` (popcount64.h) returns the number of set bits in a
64-bit word via a 256-entry byte table.  Two facts do the work, both
exact:

1. One-bit identity: `popcount(i) = (i & 1) + popcount(i >> 1)` for an
   8-bit value `i`.  `popcount_table_init()` builds the table from this
   identity alone, with `table[0] = 0`; no literal popcounts are
   hard-coded anywhere in the module.
2. Byte-decomposition sum identity: the set bits of a 64-bit word are
   partitioned by its eight bytes, so the word's popcount is the sum
   of the eight byte popcounts.  `popcount64` sums
   `table[(x >> 8k) & 0xFF]` for `k = 0..7`.

All arithmetic is unsigned; shifts reach at most bit 56 of a 64-bit
word; no UB.

## Verification plan

The reference is `__builtin_popcountll`, the compiler intrinsic
(lowered to a hardware instruction on x86_64), independent of the
table construction:

- Table sanity: all 256 table entries must equal the builtin popcount
  of their index.
- Exhaustive: all 65,536 16-bit inputs, table result vs builtin.
- Random: 1,000,000 64-bit values from splitmix64 with the fixed,
  documented seed `0x123456789ABCDEF0`, table result vs builtin.
- FNV-1a checksum over every result byte must be identical across
  `-O0`, `-O2`, and ASan+UBSan builds.
- Throughput: timed at `-O2` over 5 passes of 1M values through
  `popcount64` alone (regenerated from the same seed), accumulated
  into a volatile sink so the compiler cannot fold the loop away.

## Build log

```
$ make clean && make all
gcc -std=c11 -O2 -Wall -Wextra -Werror -o test_popcount test_popcount.c

$ make opt0
gcc -std=c11 -O0 -Wall -Wextra -Werror -o test_popcount_o0 test_popcount.c
./test_popcount_o0

$ make sanitize
gcc -std=c11 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -o test_popcount_asan test_popcount.c
./test_popcount_asan
```

Zero warnings from any build; the sanitizer runs printed no reports
(clean stderr).

## Run output

-O2:
```
table: 256 entries verified against builtin
exhaustive 16-bit: 65536 values, 0 mismatches
random 64-bit: 1000000 values (splitmix64 seed 0x123456789ABCDEF0), 0 mismatches
total checks: 1065792, mismatches: 0
fnv1a checksum: 0xe1fc995406ad5592
throughput (5 passes of 1M values, popcount64 only): best 6.975 ns/value, mean 7.405 ns/value, worst 8.706 ns/value
```

-O0:
```
table: 256 entries verified against builtin
exhaustive 16-bit: 65536 values, 0 mismatches
random 64-bit: 1000000 values (splitmix64 seed 0x123456789ABCDEF0), 0 mismatches
total checks: 1065792, mismatches: 0
fnv1a checksum: 0xe1fc995406ad5592
throughput (5 passes of 1M values, popcount64 only): best 14.641 ns/value, mean 14.943 ns/value, worst 15.536 ns/value
```

ASan+UBSan:
```
table: 256 entries verified against builtin
exhaustive 16-bit: 65536 values, 0 mismatches
random 64-bit: 1000000 values (splitmix64 seed 0x123456789ABCDEF0), 0 mismatches
total checks: 1065792, mismatches: 0
fnv1a checksum: 0xe1fc995406ad5592
throughput (5 passes of 1M values, popcount64 only): best 20.520 ns/value, mean 20.904 ns/value, worst 21.377 ns/value
```

## Claims and what each rests on

- Table construction uses only the one-bit identity: readable directly
  in `popcount_table_init()` (three lines, no literal popcounts).
- Correctness on all 16-bit inputs: the exhaustive 65,536-value run
  above, 0 mismatches against the builtin.
- Correctness on wider inputs: 1,000,000 fixed-seed 64-bit values, 0
  mismatches; the seed is fixed so the run is reproducible and the
  checksum check below would catch any divergence between builds.
- Optimization-level independence: the FNV-1a checksum over all
  1,065,792 result bytes is `0xe1fc995406ad5592` on `-O0`, `-O2`, and
  ASan+UBSan alike.
- Speed at `-O2`: mean 7.405 ns/value measured over 5 passes of 1M
  values on this machine (best 6.975, worst 8.706).

What was NOT verified: inputs above 16 bits beyond the 1M
fixed-seed sample (no exhaustive 2^64 space exists to run), and
throughput on any machine other than this VM.
