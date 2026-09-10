# lab/116-pdep-emul

Software parallel-bit-deposit for 64-bit words, `pdep64(src, mask)`:
scatter the low `popcount(mask)` bits of `src` into the set bit positions
of `mask`. The lowest remaining `src` bit lands in the lowest set mask
position, the next in the next, and so on; `src` bits beyond the low
`popcount(mask)` are ignored. This is the same scatter the x86 BMI2
`PDEP` instruction performs, computed here with shifts, masks, and a
loop.

The implementation follows the loop-scatter identity: while the mask
has set bits, take its lowest set bit (`m & (~m + 1u)`, which is `m & -m`
in unsigned arithmetic), deposit the current lowest `src` bit at that
position, shift `src` right by one, and clear the mask bit. Each
iteration therefore places the lowest remaining `src` bit into the
lowest remaining set mask position, exactly the deposit. Plain C11,
`-std=c11 -Wall -Wextra -Werror`, no intrinsics, no builtins, no library
math in the implementation.

Verified in `test_pdep.c`:

- 1,000,000 fixed-seed `splitmix64` `(src, mask)` pairs (seed
  `0x123456789ABCDEF0`), differential-checked against an independent
  naive per-bit loop reference that walks bit positions 0..63 in order
  (a different algorithm from the implementation's set-bit walk).
- All 256 8-bit mask values x 8 directed `src` values (all-zero,
  all-one, alternating runs, hex-pattern textures, byte extreme,
  top-bit row), including `mask=0`.
- `mask=all-ones` edge rows pinned in the log: with 64 set mask bits
  every `src` bit is deposited at its own position, so the result
  equals `src`.
- On every one of the 1,002,060 pairs, the conservation invariant holds:
  `popcount(result) == popcount(src & lowmask)` where `lowmask` has the
  low `popcount(mask)` bits set, so the deposit moves exactly the bits
  it should and no others.
- FNV-1a checksum `5780cdf6a1013eb2` of all results, byte-identical
  across `-O0`, `-O2`, and ASan+UBSan; zero sanitizer reports; build is
  warning-free under `-Wall -Wextra -Werror`.
- Throughput 76.59 ns/value at `-O2` (best of 5 over 25M values).
