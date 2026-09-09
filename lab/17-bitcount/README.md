# lab/17-bitcount

Population count of a 64-bit word in `bitcount.c` (`bitcount.h`
declares it), built from the SWAR parallel-add identities:

- `x - ((x >> 1) & 0x5555...)` adds each adjacent bit pair into a
  2-bit counter;
- `(x & 0x3333...) + ((x >> 2) & 0x3333...)` adds each pair of 2-bit
  counters into a 4-bit counter;
- `(x + (x >> 4)) & 0x0F0F...` adds each pair of 4-bit counters into
  a byte counter (the mask discards only bits that cannot carry a
  real count, since each field holds at most 8);
- `(x * 0x0101010101010101) >> 56` folds the eight byte counters
  into the top byte (eight bytes of at most 8 sum to at most 64,
  which fits in the top byte exactly).

No libc in the implementation, only `<stdint.h>`. The identities are
provable bit-additions; see the derivation in `bitcount.c`.

Verified by `test_bitcount.c` (fixed-seed splitmix64 PRNG, fully
reproducible), differential against the independent oracle
`__builtin_popcountll`:

- 10,000,133 total checks, 0 mismatches, identical checksums under
  `-O0`, `-O2`, and ASan+UBSan (see PROOF.md).
- 133 directed edge cases: 0, all-ones, every single-bit position
  0..63, alternating 0x5555.../0xAAAA..., and 2^k - 1 for k = 0..64.
- 10,000,000 fixed-seed random 64-bit values.

Throughput measured on 100,000,000 fresh fixed-seed values:
5.65 ns/value (177.1 Mvalues/s) at `-O0`, 3.98 ns/value
(251.4 Mvalues/s) at `-O2` (see PROOF.md for the honest caveats).

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), `make run`. See `PROOF.md` for the genuine build log
and run output.
