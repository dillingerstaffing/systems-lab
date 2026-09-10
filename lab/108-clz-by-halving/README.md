# lab/108-clz-by-halving

Count leading zeros of a `uint64_t`, `clz64_halving(x)`, from the
binary-search halving identity.

The construction: six steps, one per half size k = 32, 16, 8, 4,
2, 1. Each step tests whether the current top k bits of `x` are
zero; if so it adds k to the count and shifts `x` left by k, so the
next test again probes the top. With p the position of the top set
bit, `remaining = 63 - p - n` counts the uncounted leading zeros;
the step test `(x >> (64 - k)) == 0` holds exactly when
`remaining >= k`, and the halves 32+16+8+4+2+1 reduce any
`remaining` in 0..63 to 0, so the count ends at exactly `63 - p`.
The full invariant proof is in `clz_half.c` and `PROOF.md`.

`x = 0` returns 64 by contract (handled explicitly before any
step).

Plain C11, `-std=c11 -Wall -Wextra -Werror`, no intrinsics, no
builtins, no clz-class instruction anywhere in the implementation
(the test disassembles the `-O2` object file with `objdump` and
fails on any `bsr`, `bsf`, `lzcnt`, or `tzcnt`; the `-O2`
disassembly is `shr`/`shl`/`test`/`add`/`mov` only).
`__builtin_clzll` appears only in the test oracle, never in
`clz_half.c`.

Verified in `test_clz_half.c`:

- All 65,536 16-bit inputs, exhaustive, differential-checked
  against `__builtin_clzll`.
- The `x = 0` case checked separately against the 64 contract.
- 10,000,000 fixed-seed `splitmix64` 64-bit values (seed
  `0x123456789ABCDEF0`), each differential-checked against
  `__builtin_clzll`.
- 10,065,537 total checks, 0 mismatches. FNV-1a 64-bit checksum over
  the result stream: `bb15711279c99def`, identical across `-O0`,
  `-O2`, and ASan+UBSan builds.
- Measured at -O2: `7.56` ns/value (best of 5 reps over a 1M-value
  buffer).
- Clean under `-Wall -Wextra -Werror` with no ASan/UBSan reports.

Run `make run` in this directory for the correctness builds, `make
bench` for throughput. The genuine build log and run output are in
`PROOF.md`.
