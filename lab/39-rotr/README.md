# lab/39-rotr: 64-bit rotate right and rotate left

`rotr64(x, r)` returns `(x >> r) | (x << ((64 - r) & 63))`: the right
shift places the high bits, the left shift wraps the low `r` bits into
the top positions. The `& 63` mask keeps the complementary shift amount
in [0, 63], so `r = 0` is well defined (shifting by 64 would be
undefined in C). `rotl64(x, r)` is the mirror construction
`(x << r) | (x >> ((64 - r) & 63))`, written independently from scratch;
it does not call `rotr64` and wraps no library routine. Both reduce the
amount mod 64 on entry. No library calls in the implementation; only
shifts and ORs on `uint64_t`.

## Verification

- Differential test of `rotr64` against an independent naive bit-loop
  reference (output bit k is input bit (k + r) mod 64, rebuilt one bit
  at a time) over ALL 65536 16-bit inputs times all 32 rotation amounts
  0..31: 2,097,152 checks, 0 mismatches.
- Invariant `rotr64(rotl64(x, r), r) == x` checked on the same full
  input space: 2,097,152 checks, 0 mismatches.
- 4,194,304 total checks, 0 mismatches; identical FNV-1a checksum
  501688248194884901 across `-O0`, `-O2`, and ASan+UBSan builds.
- Clean under `-Wall -Wextra -Werror`, zero warnings; no sanitizer
  reports on the exhaustive run.

## Measurement

At `-O2`, over 100M timed values per primitive (timed loop includes one
fixed-seed splitmix64 PRNG step per value, so these are a ceiling on the
raw rotation rate):

- rotr: ~2.7 ns/value (measured 2.73, 2.77, 2.65 ns/value)
- rotl: ~2.7 ns/value (measured 2.66, 2.65, 2.65 ns/value)

See PROOF.md for the full build log and run output.
