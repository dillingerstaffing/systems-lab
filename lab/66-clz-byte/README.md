# lab/66-clz-byte

Count leading zeros of a 64-bit word, in `clz_byte.h` / `clz_byte.c`,
from a 256-entry 8-bit table plus the byte-scan identity. The contract
is `clz64_byte(0) == 64`.

For nonzero `x`, the most significant nonzero byte `B` decides: every
bit above `B` is zero (`8*(7-index(B))` of them), and `B`'s own leading
zeros within its 8 bits come from `clz8_tab[B]`, so
`clz64(x) = 8*(7-index(B)) + clz8_tab[B]`. The table itself is built at
startup from the bit-by-bit identity: for each byte value, shift left
until a 1 reaches bit 7 and count the shifts (a zero byte counts 8).
No assumed data anywhere.

Verified by `test_clz_byte.c`:

- Table check: all 256 entries re-derived by an independent bit scan
  (first 1 from bit 7 down): 0 mismatches.
- Directed checks: all 64 single-bit values plus 10 boundary patterns
  (see PROOF.md for derivations), 74 checks: 0 mismatches.
- Exhaustive differential over all 65,536 16-bit inputs against
  `__builtin_clzll` (0 mapped to 64, since the builtin is undefined
  for 0): 0 mismatches.
- Random differential: 1,000,000 splitmix64 64-bit values with the
  fixed seed `0x123456789ABCDEF0`: 0 mismatches.
- FNV-1a checksum over every result byte: `0x45C10F152E119ECB`,
  identical across `-O0`, `-O2`, and ASan+UBSan builds (all builds
  run the same full suite).
- ASan+UBSan: zero reports, exit 0.
- Throughput at `-O2`: 1.767 ns/value (566 Mops/s); the timed loop
  folds every result into a printed sink so the work cannot be
  discarded.

Build: `make run`, `make opt0`, `make sanitize`. Full evidence in
PROOF.md.
