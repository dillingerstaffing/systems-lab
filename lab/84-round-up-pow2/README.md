# lab/84-round-up-pow2

`round_up_pow2_64(x)` rounds a `uint64_t` up to the next power of two
using only the shift/OR fill cascade `x |= x >> k` for
k = 1, 2, 4, 8, 16, 32, applied to `(x - 1)`, then `+ 1`. Every step
is unsigned arithmetic, so all wrap behavior is well defined. No
builtins, no intrinsics, no tables.

## Contract decisions (pinned by dedicated test rows)

- `x = 0` maps to `0`. `x - 1` wraps to all ones, the cascade keeps
  all ones, and `+ 1` wraps back to `0`.
- Inputs already a power of two are identity: `(x - 1)` has every
  bit below the top set, the cascade changes nothing, `+ 1` restores
  `x`.
- Inputs above 2^63 map to `0`: the correct answer would be 2^64,
  which is unrepresentable in 64 bits. The cascade produces all
  ones and `+ 1` wraps to `0`. This limit is explicit, not an
  accident: the rows for `2^63 + 1`, `2^63 + 12345`,
  `UINT64_MAX - 1`, and `UINT64_MAX` all pin the observed `0`.

## What was measured

- 194 dedicated contract rows: `0 -> 0`; every `2^k` (k = 0..63)
  as identity; every `2^k - 1` (k = 2..63); every `2^k + 1`
  (k = 0..62); the four above-2^63 wrap rows. Each row asserts the
  implementation AND the independent division-loop reference against
  the same expected value derived from the identity. 0 mismatches.
- All 65,536 16-bit values, exhaustive, differential against the
  division-loop reference: 0 mismatches.
- 1,000,000 fixed-seed splitmix64 64-bit values (seed
  `0x123456789ABCDEF0`), differential against the reference:
  0 mismatches.
- The FNV-1a checksum over every case input and result,
  `0x45777d315651eb4c`, is identical across the `-O0`, `-O2`, and
  ASan+UBSan builds.
- Disassembly at `-O2` (gcc 13.3.0, x86_64): the function compiles
  to exactly the cascade, a `sub`, five `shr`/`or` pairs
  (1, 2, 4, 8, 16, 32), an `add`, and `ret`; no `tzcnt`, `bsf`,
  `popcnt`, or `lzcnt` anywhere in the object.
- Throughput at `-O2`: 2.27 ns/value (best of 5 over the 1,000,000
  timed values, PRNG generation outside the timed region).

`-std=c11 -Wall -Wextra -Werror` clean; zero ASan/UBSan reports
across the full case set. Exact build logs and run output are in
PROOF.md.
