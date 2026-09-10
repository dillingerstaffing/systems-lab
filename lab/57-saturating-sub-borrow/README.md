# lab/57-saturating-sub-borrow

`sat_sub64(a, b)`: unsigned saturating subtraction of two `uint64_t`
values. Returns `a - b` when `a >= b`, and 0 when `a < b`.

Why the construction holds: for unsigned operands, the subtraction
`a - b` underflows exactly when `a < b`, so the borrow-out of `a - b`
is precisely the value of the comparison `a < b`; there is no carry
chain to reason about. Saturation then reduces to masking the raw
difference to zero on borrow: `keep = -(uint64_t)(a >= b)` is all-ones
when `a >= b` and zero otherwise, so `diff & keep` is the saturated
result. All arithmetic is unsigned, so the wrap in `a - b` is defined
by the C standard. The implementation contains no branch (checked in
the generated assembly: `sub; cmp; cmovb; ret` at `-O2`, no
conditional jumps; `setae`/`neg` at `-O0`) and no `__int128`.

Files: `sat_sub.c`, `sat_sub.h` (this one function only),
`test_sat_sub.c` (see `PROOF.md` for the build log and measured
results).

- `test_sat_sub.c`
  - Exhaustive over all 2^16 x 2^16 = 4,294,967,296 `uint16_t` pairs,
    differential-checked against the exact reference
    `(a >= b) ? (uint64_t)((unsigned __int128)a - b) : 0` (ran on the
    `-O2` build). 0 mismatches, 325.9 s (75.888 ns/pair, includes the
    oracle and checksum per pair).
  - Directed 228-pair sweep: 12 edge values
    `{0, 1, 2, 3, 2^32-1, 2^32, 0x5555..., 0xAAAA..., 2^63-1, 2^63,
    2^64-2, 2^64-1}` cross-checked (144 pairs) plus a boundary band
    around each edge value (`b` in `{0, a-2, a-1, a, a+1, a+2,
    UINT64_MAX}`, 84 pairs), checked against the same reference.
    0 mismatches.
  - 10,000,000 fixed-seed splitmix64 random 64-bit pairs
    (seed 0x243F6A8885A308D3, reproducible). Ran on all three builds
    (`-O0`, `-O2`, `-fsanitize=address,undefined`). 0 mismatches.
  - FNV-1a checksum over the directed and random phases:
    0xfb3dd8407f4dd8d1, identical across all three builds.
  - Throughput (`-O2`, 100,000,000 timed pairs): 4.300 ns/pair,
    including the splitmix64 PRNG step per pair, so this is a lower
    bound on the saturating subtract itself.

Total: 4,304,967,524 checked pairs, 0 mismatches.

Build: `make all` compiles the test under `-O0`, `-O2`, and
`-fsanitize=address,undefined` with `-std=c11 -Wall -Wextra -Werror`,
zero warnings on all three.
