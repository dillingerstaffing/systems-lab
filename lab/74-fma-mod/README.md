# lab/74-fma-mod

`fma_mod64(a, b, c)` in `fma_mod.h`: the exact value `(a * b + c) mod
2^64`, computed from the 32-bit splitting identities only.  With `a =
ah * 2^32 + al` and `b = bh * 2^32 + bl`,

    a * b = p3 * 2^64 + (p1 + p2) * 2^32 + p0,

where `p0 = al*bl`, `p1 = al*bh`, `p2 = ah*bl`, `p3 = ah*bh` are the
four 32x32 -> 64 partial products, each exact in 64 bits.  The middle
column `col1 = p1 + p2 + (p0 >> 32)` is accumulated in a 64-bit
register with explicit wrap detection (`(x + y) < x` is 1 exactly when
the addition wrapped), giving `col1 = (k1 + k2) * 2^64 + mid2`
exactly.  Modulo 2^64 the terms in `p3` and `(k1 + k2)` vanish, so the
low 64 bits of `a * b` are exactly `(uint32_t)p0 +
(((uint32_t)mid2) << 32)`, a sum below 2^64 that cannot wrap; `c` is
then added with unsigned wraparound, which is addition mod 2^64.  No
`__int128` in the implementation, no intrinsics, no builtins.

Verified by `test_fma_mod.c`, differential against the `unsigned
__int128` oracle `((unsigned __int128)a * b + c)` truncated to 64
bits (only the oracle may use `__int128`):

- Anchors: 7 hand-checked values, including `(2^64 - 1)^2 == 1`
  (mod 2^64) and `(2^64 - 1)^2 + (2^64 - 1) == 0`.
- Exhaustive: all 2^32 pairs `(a, b)` with `a, b < 2^16`, each with 8
  fixed `c` values (0, 1, 2, 2^32 - 1, 2^32, 2^64 - 1,
  2^63, 0x123456789ABCDEF0): 34,359,738,368 checks.  (With `ah = bh =
  0` this exhaustively covers the `p0 = al * bl` partial-product path
  and the final `+ c` addition.)
- Directed: 26 edge triples (all-zeros, all-ones, half-word
  boundaries, `+c` wrap boundaries, triples constructed to force the
  middle-column carries `k1` and `k2`) plus 192 single-bit sweeps.
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0xF1A074F74A04D0D`) 64-bit triples.
- 34,369,738,593 checks per build, 0 mismatches, in each of the `-O0`,
  `-O2`, and ASan+UBSan builds.
- Carry coverage, tallied independently with `unsigned __int128`:
  `k1` (p1+p2 wraps) fired 4 times in the directed set and 725,371
  times over the 10M random triples; `k2` (mid + (p0>>32) wraps)
  fired twice in the directed set and 0 times over the random
  triples (it needs `mid` within 2^32 of the 2^64 boundary, rare
  under random inputs, so it is covered by constructed triples).
- FNV-1a checksum over every output word is identical across all
  three builds: `0xee898af0ca125a96`.
- Throughput at `-O2`: best of 5 passes over 2M pre-generated
  triples, 2.144 ns/triple (466.320 M triples/s).  The triples are
  generated once with splitmix64 before timing starts, so the PRNG
  is not part of the measured loop; results are xored into a
  volatile sink.
- Disassembly at `-O2`: the 32-bit splitting survives (three `imul`
  partial products, shift/accumulate, final add); it is not folded
  into a single 64-bit multiply.  The carry-bit comparisons and the
  discarded high word are eliminated as dead code, which is
  legitimate: they feed only the high 64 bits that `mod 2^64`
  discards.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full suite.
