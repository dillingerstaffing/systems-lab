# lab/33-gray-code: 16-bit Gray code encode and decode

`gray16_encode(n)` returns `n ^ (n >> 1)`: XORing every adjacent bit pair
at once, which is bit k of the Gray code being `n_k XOR n_{k+1}`. The
decode inverts it by xor-folding (`g ^= g>>8; g ^= g>>4; g ^= g>>2;
g ^= g>>1`): each fold propagates the already-decoded high bits into the
low half, and four steps cover all 16 positions. No library calls or
lookup tables in the implementation; only shifts and XORs on `uint16_t`.

## Verification

- Differential test against an independent naive bit-loop reference over
  ALL 65536 16-bit inputs, for both encode and decode: 65,536 checks
  each, 0 mismatches.
- Involution `decode(encode(x)) == x` checked on every input: 65,536
  checks, 0 mismatches.
- Single-bit adjacency `popcount(encode(x) ^ encode(x + 1)) == 1` checked
  for all 65,535 adjacent pairs: 0 mismatches.
- 262,143 total checks, 0 mismatches; identical FNV-1a checksum
  9751602672369123877 across `-O0`, `-O2`, and ASan+UBSan builds.
- Clean under `-Wall -Wextra -Werror`, zero warnings; no sanitizer
  reports.

## Measurement

At `-O2`, over 100M timed values per primitive (timed loop includes one
fixed-seed splitmix64 PRNG step per value, so these are a ceiling on the
raw rate):

- encode: ~4.3 ns/value (measured 4.46, 4.32, 4.35 ns/value)
- decode: ~4.3 ns/value (measured 4.35, 4.33, 4.31 ns/value)

See PROOF.md for the full build log and run output.
