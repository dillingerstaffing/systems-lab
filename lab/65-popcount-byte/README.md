# lab/65-popcount-byte

`popcount64(x)` in `popcount64.h`: population count of a 64-bit word
using a 256-entry byte lookup table.  The table is built at startup
from the one-bit identity `table[i] = (i & 1) + table[i >> 1]` with
`table[0] = 0`; no popcount is hard-coded anywhere.  The word result is
the sum of the eight byte table entries, since the set bits of the
word are partitioned by its eight bytes.

Verified by `test_popcount.c` against `__builtin_popcountll`:

- Table sanity: all 256 entries equal the builtin popcount of their index.
- Exhaustive: all 65,536 16-bit inputs, differential against the builtin.
- Random: 1,000,000 fixed-seed (splitmix64, seed `0x123456789ABCDEF0`)
  64-bit values, differential against the builtin.
- 1,065,792 checks total, 0 mismatches.
- FNV-1a checksum over every result byte: identical
  (`0xe1fc995406ad5592`) across `-O0`, `-O2`, and ASan+UBSan builds.
- Throughput at `-O2`: mean 7.405 ns/value over 5 passes of 1M values
  (popcount64 only, accumulation sink volatile so the loop is not
  folded away). Zero warnings under `-std=c11 -Wall -Wextra -Werror`;
  clean under ASan and UBSan.
