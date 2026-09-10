# lab/76-ceil-log2

`ceil_log2_64(x)` in `ceil_log2.h`: the smallest n with 2^n >= x,
for a 64-bit word, built from three bit identities:

1. `floor_log2` from the shift/OR fill propagation cascade
   (`x |= x >> k` for k = 1, 2, 4, 8, 16, 32), which leaves
   exactly `floor + 1` bits set; the result is a SWAR popcount
   minus 1.
2. The power-of-two test `(x & (x - 1)) == 0`, which clears the
   lowest set bit and is 0 iff exactly one bit was set.
3. `ceil = floor + (x not a power of two ? 1 : 0)`, exact because
   the power-of-two and non-power-of-two cases partition every
   x >= 1.

No builtins, no intrinsics, no inline asm.

Zero-input contract: the domain is x >= 1.  `ceil_log2_64(0)`
returns 0 by explicit contract; the guard sits before the fill
cascade (whose popcount-minus-1 would wrap on the empty input).
The contract is pinned by a test row and kept out of the
differential set.

Verified by `test_ceil_log2.c`, differential against an
independent exact doubling-loop reference (the loop widens p to
UINT64_MAX before overflow, so its n is exact through 2^64 - 1):

- Anchors: 15 hand-checked values plus the x = 0 contract row.
- Exhaustive: all 65,536 16-bit inputs.
- Boundary rows: 2^k, 2^k - 1, 2^k + 1 for k = 0..63 (all fit
  in 64 bits; the k = 0, 2^k - 1 = 0 case is the contract row).
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0x123456789ABCDEF0`) 64-bit words.
- 10,065,744 checks per build, 0 mismatches, in each of the
  `-O0`, `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every output count is identical across
  all three builds: `0x52130d09b192774b`.
- Throughput at `-O2`: best of 50 passes over 2M pre-generated
  words (100M total timed values), 6.047 ns/value
  (165.367 M values/s); with the splitmix64 PRNG inside the timed
  loop, 10.775 ns/value.
- Disassembly at `-O2`: no `bsr`, `lzcnt`, `tzcnt`, or `popcnt`
  anywhere in the binary.  The fill cascade survives as
  `shr`/`or` pairs and the SWAR popcount survives with its mask
  constants (e.g. 0x5555555555555555 as `movabs` immediates);
  gcc 13.3.0 does not fold either part into a count-class
  instruction.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean
  under AddressSanitizer and UBSan on the full suite.
