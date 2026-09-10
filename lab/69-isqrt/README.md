# lab/69-isqrt: 64-bit integer square root

`isqrt.h` implements `isqrt64`, the integer square root of a `uint64_t`
(the largest `r` with `r*r <= n`), using only the digit-by-digit
restoring square root recurrence: 32 iterations, each deciding one more
bit of the answer from the most significant bit down, testing the trial
bit with unsigned add, subtract, compare, and shift. No floating point,
no libm, no compiler sqrt builtins.

## What was verified

- Exhaustive invariant over all 2^32 32-bit inputs (4,294,967,296 cases):
  for each `x`, `r = isqrt64(x)` satisfies `r*r <= x < (r+1)*(r+1)` in
  64-bit arithmetic. 0 violations. FNV-1a checksum over every result:
  `f0bc609332350325`, identical across the -O0, -O2, and ASan+UBSan
  builds.
- Differential test against a naive binary-search reference (written with
  the overflow-free test `m <= x/m`, no float anywhere): all 2^16 inputs
  plus 1,000,000 fixed-seed splitmix64 64-bit values (seed
  `0x123456789ABCDEF0`). 0 mismatches. The 64-bit samples also satisfy
  the invariant checked without overflow (`r*r <= x` and
  `r+1 > x/(r+1)`).
- Edge cases (30): `x = 0` explicitly, small values, perfect squares and
  their neighbors, `2^32-1`, `2^32`, `2^32+1`, `(2^32-1)^2` and
  `(2^32-1)^2 - 1`, `2^63-1`, `2^63`, `2^64-2`, `2^64-1`. All match the
  reference.
- Throughput at -O2: 267.73 ns/value on an uncontended run; repeated runs
  on this shared 2-core VM measured 267-666 ns/value as CPU frequency
  scaling and a concurrent worker varied. The timed loop measures
  `isqrt64` only: the input array is precomputed, so no PRNG step runs
  inside the timed loop.
- Builds clean under `-std=c11 -Wall -Wextra -Werror` at -O0 and -O2,
  and under ASan+UBSan (`-O1 -g`), with zero sanitizer findings on the
  full run.

See PROOF.md for the verbatim build log and run output.
