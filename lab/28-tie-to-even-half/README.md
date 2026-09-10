# lab/28-tie-to-even-half

`tie_to_even_half(x)` rounds `x / 2` to the nearest integer with ties
(odd `x`) resolved to the even neighbor, using only the bit identity

    q = x >> 1
    result = q + ((x & 1) & (q & 1))

When `x` is even, `x & 1` is 0 and the result is exactly `q`. When `x`
is odd, `x = 2q + 1` sits exactly halfway between `q` and `q + 1`, and
the tie goes to the even of the two, so `q` rounds up only when `q`
itself is odd. All operations are unsigned 32-bit: two shifts, two
ANDs, one add. No floats, no division. The result is always `<= x`.

## Contract decisions (pinned by dedicated test rows)

- Even inputs return exactly `x / 2`.
- Odd inputs are the exact midpoint `2q + 1` between `q` and `q + 1`;
  the result is whichever of the two is even: `1 -> 0`, `3 -> 2`,
  `5 -> 2`, `7 -> 4`.
- Boundary rows: `0x80000000 -> 0x40000000` (exact), `0x80000001 ->
  0x40000000` (tie, `q` even), `0x80000003 -> 0x40000002` (tie, `q`
  odd), `0xFFFFFFFE -> 0x7FFFFFFF` (exact), `0xFFFFFFFF -> 0x80000000`
  (tie, `q` odd).

## What was measured

- 13 dedicated contract rows, each asserted against both the
  implementation and the independent quotient/remainder reference
  with the same hand-derived expectation: 0 mismatches.
- Exhaustive over ALL 4,294,967,296 `uint32_t` values, differential
  against the independent reference `q = x / 2, r = x % 2`
  (computed by the division unit, a separate path from the
  shift/AND identity): 0 mismatches.
- The FNV-1a checksum over every case input and result,
  `0x5ae86dd58b5d3a01`, is identical across the `-O0`, `-O2`, and
  ASan+UBSan builds.
- Disassembly at `-O2` (gcc 13.3.0, x86_64): `shr`, `and`, `and`,
  `add`, `ret`; no `div`/`idiv` anywhere in the object.
- Throughput at `-O2`: 2.27 ns/value (best of 5 over 4,194,304
  fixed inputs, PRNG generation outside the timed region).

`-std=c11 -Wall -Wextra -Werror` clean; zero ASan/UBSan reports
across the full case set. Exact build logs and run output are in
`PROOF.md`.
