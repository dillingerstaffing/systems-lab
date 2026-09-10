# lab/75-leading-ones

`clo64(x)` in `leading_ones.h`: the count of consecutive 1 bits from
the most significant bit of a 64-bit word, from the invert-then-scan
identity `clo(x) = clz(~x)`.  The leading-zero count is the
shift/OR fill propagation cascade (`y |= y >> k` for k = 1, 2, 4, 8,
16, 32), which fills every bit at or below the highest set bit of
`~x`; the result is `~y` counted with a SWAR popcount.  No clz-class
instruction, no builtins, no intrinsics, no inline asm.

Zero-input contract: the degenerate input of the scan is `~x = 0`,
i.e. `x = 0xFFFFFFFFFFFFFFFF`, which the cascade maps to 0 and the
popcount of `~0` to 64, so `clo(0xFFFFFFFFFFFFFFFF) = 64` with no
special case.  `x = 0` itself has zero leading ones and returns 0.

Verified by `test_leading_ones.c`, differential against a naive
bit-by-bit reference scan:

- Anchors: 10 hand-checked values, including `clo(0) = 0` and the
  `clo(0xFFFFFFFFFFFFFFFF) = 64` zero-input contract.
- Exhaustive: all 65,536 16-bit inputs.
- Directed: for every run length k = 0..64, the word whose top k
  bits are 1 and the rest 0 (every possible count has an exact
  witness), plus 8 alternating/boundary patterns.
- Random: 10,000,000 fixed-seed (splitmix64, seed
  `0xC10DCAFE12345678`) 64-bit words.
- 10,065,619 checks per build, 0 mismatches, in each of the `-O0`,
  `-O2`, and ASan+UBSan builds.
- FNV-1a checksum over every output count is identical across all
  three builds: `0x89c5b240295bdfcc`.
- Throughput at `-O2`: best of 5 passes over 2M pre-generated
  words, 5.483 ns/value (182.386 M values/s); with the splitmix64
  PRNG inside the timed loop, 8.267 ns/value.
- Disassembly at `-O2`: no `bsr`, `lzcnt`, or `popcnt` anywhere.
  The fill cascade survives as six `shr`/`or` pairs, the SWAR
  popcount survives as shifts/masks/adds with one `imul` for the
  0x0101010101010101 byte-sum step; gcc 13.3.0 does not fold either
  part into a count-class instruction.
- Zero warnings under `-std=c11 -Wall -Wextra -Werror`; clean under
  AddressSanitizer and UBSan on the full suite.
