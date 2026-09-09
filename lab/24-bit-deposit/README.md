# lab/24-bit-deposit

`bit_deposit.c` (`bit_deposit.h` declares them): bitfield extract and
insert for 64-bit words, `uint64_t bit_extract(uint64_t x, unsigned off,
unsigned w)` and `uint64_t bit_insert(uint64_t x, unsigned off,
unsigned w, uint64_t v)`. Both are built only from the shift/mask
identities: `mask_n = (1ULL << n) - 1` (with the n = 64 special case,
since `1ULL << 64` is undefined), `extract(x, off, w) =
(x >> off) & mask_w`, and `insert(x, off, w, v) =
(x & ~(mask_w << off)) | ((v & mask_w) << off)`. No loops, no
conditionals on the data, no library calls in either primitive.

Domain convention: off + w <= 64 (for w = 64, off must be 0). w = 0
is a defined no-op: extract returns 0, insert returns x unchanged.

Verified by `test_bit_deposit.c` (fixed-seed splitmix64, seed
`0x123456789ABCDEF0`, fully reproducible):

- Exhaustive differential against a naive per-bit loop reference
  (bit k of the field checked against bit off + k of the word):
  every width 1..8, every offset 0..15, all 65536 16-bit inputs, for
  both extract and insert. 8 * 16 * 65536 * 2 = 16,777,216 checks,
  0 mismatches.
- Round-trip invariant over the full 64-bit domain: inserting the
  extracted field back into a zero word must reproduce exactly the
  field bits of x and clear everything else, i.e.
  `bit_insert(0, off, w, bit_extract(x, off, w)) == x & (mask_w << off)`.
  All widths 1..63, 1,000,000 fixed-seed random values each with the
  offset drawn from the same stream. 63,000,000 checks, 0 mismatches.
- 79,777,216 total checks, 0 mismatches. Identical FNV-1a checksum
  (`1148145629208527209`) under `-O0`, `-O2`, and ASan+UBSan; zero
  sanitizer reports.

Timing at `-O2` (100,000,000 timed iterations, one extract plus one
insert per iteration on PRNG-fed values, XORed into a sink so the
loop cannot be optimized away): 5.08 ns/iteration. Honest caveats:
each iteration includes one PRNG step, so the figure is the cost of
one generate-extract-insert case, not bare primitive calls, and the
sink total (`3488470406996442993`) was identical across all three
builds, confirming the timed code paths agree.

Build: `make` (`-O0`, `-O2`, ASan+UBSan, `-Wall -Wextra -Werror`,
zero warnings), then run the three binaries in turn. See `PROOF.md`
for the genuine build log and run output.
