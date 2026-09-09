# lab/26-msb-lsb: find-first-set and find-last-set for 64-bit words

`ffs64(x)` returns the 1-based index of the lowest set bit (0 when x == 0),
`fls64(x)` the 0-based index of the highest set bit (-1 when x == 0).

## Construction

Both functions are built from two bit identities, with no bit-scan
builtins and no library calls in the implementation:

- `x & -x` isolates the lowest set bit of a nonzero unsigned x, producing
  exactly one of the 64 powers 2^k (two's complement fact).
- The shift/OR smear (`x |= x>>1; x |= x>>2; ... x |= x>>32`) propagates the
  highest set bit downward, producing exactly one of the 64 all-ones
  prefixes 2^(k+1) - 1.

Each set of 64 values is mapped through one de Bruijn multiply
(`* 0x03f79d71b4cb0a89`, keep the top 6 bits) to 64 distinct hash values,
and a 64-entry table inverts each hash back to the bit index. The test
binary asserts the 64 hashes are pairwise distinct for both tables, so
the constant's de Bruijn property is verified by the test, not trusted.
All arithmetic is unsigned, all shifts are in range, so there is no
undefined behavior.

## Verification

- Differential test: `ffs64` against `__builtin_ffsll`, `fls64` against
  `63 - __builtin_clzll`, over all 65536 16-bit inputs plus 133 directed
  edge cases (0, all-ones, 0xAAAAAAAAAAAAAAAA, 0x5555555555555555, every
  single bit 2^k, every prefix 2^(k+1)-1): 131,335 checks, 0 mismatches.
- Identical FNV-1a checksum 7928615795610640929 across `-O0`, `-O2`, and
  ASan+UBSan builds.
- Clean under `-Wall -Wextra -Werror`, zero warnings; no sanitizer
  reports.
- Disassembly of the `-O2` build confirms the multiply-and-table
  construction survives compilation unchanged: no `tzcnt`, `lzcnt`,
  `bsr`, `bsf`, or `popcnt` anywhere in `ffs64` or `fls64`.

## Measurement

At `-O2`, over 100M timed values per function (timed loop includes one
fixed-seed PRNG step per value, so these are a ceiling on the raw rate):

- ffs64: 3.74 ns/value
- fls64: 4.35 ns/value

See PROOF.md for the full build log and run output.
