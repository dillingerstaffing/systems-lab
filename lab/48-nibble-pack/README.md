# lab/48-nibble-pack: 16 4-bit nibbles packed into one 64-bit word

`pack_nibbles16()` (nibblepack.h) packs 16 4-bit nibbles into one 64-bit
word by the identity `word |= (uint64_t)(n[i] & 0xF) << (4*i)`: nibble `i`
occupies bits `[4*i, 4*i+3]`, so each nibble lands exactly on one hex
digit of the word. `unpack_nibbles16()` inverts it with
`out[i] = (w >> (4*i)) & 0xF`. Shift/OR/mask composition only: no tables,
no library bit tricks. Inputs above 0xF are reduced to their low 4 bits.

## Files

- `nibblepack.h`: the implementation (two functions).
- `test_nibblepack.c`: five known-answer vectors, an exhaustive
  differential test against an independent bit-level reference, a 1M-word
  round-trip test on both directions, an FNV-1a checksum over all packed
  outputs, and a throughput benchmark.
- `Makefile`: `test-o0`, `test-o2`, `test-asan`, `test-ubsan`, `clean`.
- `PROOF.md`: genuine build log and measured numbers.

## Measured

- 5/5 vectors pass: all-zero -> 0, all-0xF -> 0xFFFFFFFFFFFFFFFF,
  nibbles 0..15 -> 0xFEDCBA9876543210 (hand-checked: the hex digit at
  position i, counting from the least significant digit, equals i), wide
  inputs reduced to low 4 bits ({0xFF,0x10,0x21,0xAB,...} ->
  0x00EDCBA9870FB10F), and unpack(0xFEDCBA9876543210) -> 0..15.
- Differential: all 65,536 16-bit values decomposed into 4 nibbles and
  packed into each of the 4 nibble lanes (bits 0..15, 16..31, 32..47,
  48..63), checked against an independent reference that builds each word
  bit by bit from a 16x4 truth table and literal place values using only
  multiply, add, and divide (no shift, OR, or mask shared with the
  implementation): 262,144 pack checks and 262,144 unpack checks, 0
  mismatches.
- Round trip on 1,000,000 splitmix64 words (fixed seed
  0x123456789ABCDEF0): unpack(pack(n)) == n for the nibble arrays and
  pack(unpack(w)) == w for the words, 0 mismatches.
- FNV-1a fingerprint 0x55bf0e9a9dad34f2 over all packed outputs, identical
  across -O0, -O2, ASan+UBSan, and UBSan builds.
- Clean compile with `-std=c11 -Wall -Wextra -Werror` at -O0, -O2, and
  both sanitizer builds; zero warnings, zero sanitizer reports (built
  with `-fno-sanitize-recover=all`, so any report would be fatal).
- Throughput at -O2: 2,000,000 pack+unpack pairs in 0.055 s = 27.7
  ns/pair. See PROOF.md for the full build and run logs.
