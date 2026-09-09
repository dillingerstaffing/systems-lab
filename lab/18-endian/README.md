# lab/18-endian

Byte-swap primitives in `endian.c` (`endian.h` declares them):

- `u16_swap`: `(x << 8) | (x >> 8)` after masking each byte lane;
- `u32_swap`: swap adjacent bytes inside each 2-byte pair, then swap
  the two pairs;
- `u64_swap`: swap adjacent bytes, then adjacent 2-byte pairs, then
  the two 4-byte halves.

Only shifts, ORs, and masks. No builtin, no libc bswap, no inline
assembly. Each byte's destination lane is fixed by the permutation,
and a shift by a whole multiple of 8 moves a byte from lane to lane
exactly; the mask isolates one lane before the shift so the shifted
pieces are disjoint and OR merges them without overlap. The staged
forms compose the same permutation: after each stage, every byte is
one step closer to its mirror lane.

Verified by `test_endian.c` (fixed-seed splitmix64 PRNG, fully
reproducible), differential against the independent oracles
`__builtin_bswap16/32/64`, plus the involution invariant
`swap(swap(x)) == x` on every value:

- 12,000,304 total checks, 0 mismatches, identical checksums under
  `-O0`, `-O2`, and ASan+UBSan (see PROOF.md).
- 152 directed values (304 checks): 0, all-ones, each byte position
  set, each bit position set 0..15/31/63, `0x01020304`-style patterns,
  high/low halves, and mixed masks per width.
- 2,000,000 fixed-seed random values per width (6,000,000 total),
  both checks each.

Throughput measured on 100,000,000 fresh fixed-seed values through
`u64_swap`: 5.37 ns/value (186.1 Mvalues/s) at `-O0`, 2.74 ns/value
(365.6 Mvalues/s) at `-O2` (see PROOF.md for the honest caveats).

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), `make run`. See `PROOF.md` for the genuine build log
and run output.
