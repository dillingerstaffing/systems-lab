# lab/59-avg-no-overflow

`avg_u64`: average (floor of `(a + b) / 2`) of two `uint64_t` values,
computed as `(a & b) + ((a ^ b) >> 1)` without ever forming `a + b`.

Why the identity holds: `a + b = ((a & b) << 1) + (a ^ b)` as ordinary
integers, because the bits where both inputs are 1 contribute 2 each and
the bits where exactly one is 1 contribute 1 each. Halving gives
`floor((a + b) / 2) = (a & b) + ((a ^ b) >> 1)`. The sum in the return
statement equals `floor((a + b) / 2) <= 2^64 - 1` exactly, so it cannot
wrap; all operations are unsigned, so no undefined behavior is possible.

Header-only: `avg.h`. Tests: `test_avg.c` (see `PROOF.md` for the build
log and measured results).

- `test_avg.c`
  - Exhaustive over all 2^16 x 2^16 = 4,294,967,296 `uint16_t` pairs,
    differential-checked against the exact reference
    `(uint64_t)(((unsigned __int128)a + b) / 2)` (ran on the `-O2` build).
  - Directed 144-pair edge sweep over
    `{0, 1, 2, 3, 2^32-1, 2^32, 0x5555..., 0xAAAA..., 2^63-1, 2^63,
    2^64-2, 2^64-1}`, checked against the same reference.
  - 10,000,000 fixed-seed splitmix64 random 64-bit pairs, checked against
    the same reference. Ran on all three builds (`-O0`, `-O2`,
    `-fsanitize=address,undefined`).
