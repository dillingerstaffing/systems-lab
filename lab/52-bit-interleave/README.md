# lab/52-bit-interleave

Morton (Z-order) codes for a 16-bit coordinate pair. `morton_interleave`
spreads the 16 bits of each input into alternating positions of a 32-bit
word (bits of `lo` in even positions, bits of `hi` in odd positions);
`morton_deinterleave` is the exact inverse, packing even-position bits
into the low 16 bits and odd-position bits into the high 16 bits.
Both are built only from the bit-spreading shift/mask identities
(`x = (x ^ (x << 8)) & 0x00FF00FF`, and so on down to
`& 0x55555555`); no lookup tables, no builtins. All arithmetic is on
`uint32_t`, so every shift is a logical shift with no overflow risk.
Header-only: `morton.h`. Tests: `test_morton.c`.

- `test_morton.c`
  - Exhaustive over all 2^8 x 2^8 = 65,536 `(lo, hi)` pairs, each
    differential-checked on both directions against a naive per-bit-loop
    reference, plus the `deinterleave(interleave(lo, hi))` field check:
    196,608 checks.
  - 10,000,000 fixed-seed splitmix64 random 32-bit values, split into
    `(lo, hi)` halves; the round-trip `deinterleave(interleave(v)) == v`
    invariant asserted on every value, plus a deinterleave differential
    against the naive reference: 20,000,000 checks.
  - Total: 20,196,608 checks, 0 mismatches, at `-O0`, `-O2`, and under
    ASan+UBSan; FNV-1a checksum `0x57707ad2ff081ccb` identical in all
    three builds; zero warnings, zero sanitizer reports.
  - Benchmark: timed round-trip loop of 50,000,000 pairs at `-O2`,
    reported in ns/pair in PROOF.md (8.705 ns/pair; the loop includes the
    splitmix64 step, so this is a ceiling).
