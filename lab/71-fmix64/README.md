# lab/71-fmix64

`fmix64(k)` in `fmix64.h`: the 64-bit finalization mix used by
MurmurHash3_x64_128,

    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;

plus its exact inverse `fmix64_inv`, built step by step in reverse:

- The shift-xor step `y = x ^ (x >> 33)` is inverted by repeated
  folding `x ^= x >> s` with s doubling (33, then 66 >= 64 exits):
  the low 33 bits of y are already correct, and each fold fixes the
  next block, since `y[i..2i) ^ x[0..i) = x_true[i..2i)`.
- The multiply constants are odd, so each is a unit modulo 2^64.
  Their inverses were found by Newton-Raphson iteration
  `inv <- inv * (2 - c * inv)` (six rounds: 1 -> 2 -> 4 -> 8 -> 16 ->
  32 -> 64 correct bits), verified by `inv * c == 1` (mod 2^64):
  - inv(0xff51afd7ed558ccd) = 0x4f74430c22a54005
  - inv(0xc4ceb9fe1a85ec53) = 0x9cb4b2f8129337db

Every step is a bijection on 64-bit words, so the reverse-order
composition satisfies `fmix64_inv(fmix64(x)) == x` for all x.

Verified by `test_fmix64.c` (differential reference in
`fmix64_ref.c`, the published MurmurHash3 source text with the symbol
renamed so both can link into one binary):

- Anchor: `fmix64(0) == 0`, since each of the five steps fixes 0.
- Inverse constants: independently re-derived in the test by
  Newton-Raphson from inv = 1, matching the header constants, and
  `inv * c == 1` for both.
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0x123456789ABCDEF0`) 64-bit values; every output compared to the
  published reference, and `fmix64_inv(fmix64(x)) == x` checked on
  every value: 0 differential mismatches, 0 bijection mismatches.
- Directed: 32 structurally extreme words (zero, single bits on both
  sides of the 33-bit shift boundary, alternating patterns,
  half-word boundaries, the two constants and their neighbors) plus
  all 64 single-bit words round-tripped both directions.
- 10,000,162 checks per build, 0 mismatches, in each of the `-O0`,
  `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every output word is identical across all
  three builds: `0xa893938fd43cc2de`.
- Throughput at `-O2`: best of 5 passes of 4M values, 1.491
  ns/value (670.480 M values/s). The 4M values are generated once
  with splitmix64 before timing starts, so the PRNG is not part of
  the measured loop; results are xored into a volatile sink.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full suite.
